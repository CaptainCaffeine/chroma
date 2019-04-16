// This file is a part of Chroma.
// Copyright (C) 2018-2019 Matthew Murray
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>
#include <fmt/format.h>

#include "gba/lcd/Lcd.h"
#include "gba/lcd/Bg.h"
#include "common/Screenshot.h"

namespace Gba {

void Lcd::DumpDebugInfo() const {
    for (const Bg& bg : bgs) {
        bg.DumpBg();
    }

    DumpSprites();

    DumpTileset(0, false);
    DumpTileset(32 * 1024, false);
    DumpTileset(64 * 1024, false);

    if (std::any_of(bgs.cbegin(), bgs.cend(), [](const Bg& bg) { return bg.SinglePalette(); })
            || BgMode() > 0) {
        DumpTileset(0, true);
    }
}

void Lcd::DumpSprites() const {
    std::vector<u16> sprite_buffer;
    for (std::size_t s = 0; s < sprites.size(); ++s) {
        const auto& sprite = sprites[s];

        sprite_buffer.resize(sprite.pixel_width * sprite.pixel_height);

        int vertical_index = 0;
        while (vertical_index < sprite.pixel_height) {
            int scanline_index = 0;
            int tile_row = vertical_index / 8;
            int pixel_row = vertical_index % 8;

            if (sprite.v_flip) {
                tile_row = (sprite.tile_height - 1) - tile_row;
                pixel_row = 7 - pixel_row;
            }

            int tile_index = tile_row * sprite.tile_width;
            int tile_direction = 1;

            if (sprite.h_flip) {
                tile_index = (tile_row + 1) * sprite.tile_width - 1;
                tile_direction = -1;
            }

            while (scanline_index < sprite.pixel_width) {
                int tile_addr = sprite.tile_base_addr + tile_index * sprite.tile_bytes;
                if (ObjMapping2D()) {
                    const int h = tile_index / sprite.tile_width;
                    tile_addr += h * sprite.tile_bytes * ((sprite.single_palette ? 16 : 32) - sprite.tile_width);
                }
                tile_index += tile_direction;

                const std::array<u16, 8> pixel_colours = GetTilePixels(tile_addr, sprite.single_palette, sprite.h_flip,
                                                                       pixel_row, sprite.palette, 256);

                for (int i = 0; i < 8; ++i) {
                    sprite_buffer[vertical_index * sprite.pixel_width + scanline_index++] = pixel_colours[i];
                }
            }

            vertical_index += 1;
        }

        Common::WriteImageToFile(Common::BGR5ToRGB8(sprite_buffer), fmt::format("sprite{}", s),
                                 sprite.pixel_width, sprite.pixel_height);
    }
}

std::vector<BgTile> Bg::ReadEntireTileMap() const {
    std::vector<BgTile> all_tiles;

    // Get all of the map entries from the specified screenblock.
    auto ReadMap = [this, &all_tiles](int screenblock) {
        const int map_addr = (MapBase() + 0x800 * screenblock) / 2;
        const int tile_bytes = SinglePalette() ? 64 : 32;

        for (int i = 0; i < 1024; ++i) {
            all_tiles.emplace_back(lcd.vram[map_addr + i], TileBase(), tile_bytes);
        }
    };

    auto ReadHorizontalMap = [this, &all_tiles](int screenblock) {
        const int map_addr0 = (MapBase() + 0x800 * screenblock) / 2;
        const int map_addr1 = (MapBase() + 0x800 * (screenblock + 1)) / 2;
        const int tile_bytes = SinglePalette() ? 64 : 32;

        for (int j = 0; j < 32; ++j) {
            for (int i = 0; i < 32; ++i) {
                all_tiles.emplace_back(lcd.vram[map_addr0 + j * 32 + i], TileBase(), tile_bytes);
            }
            for (int i = 0; i < 32; ++i) {
                all_tiles.emplace_back(lcd.vram[map_addr1 + j * 32 + i], TileBase(), tile_bytes);
            }
        }
    };

    switch (ScreenSize()) {
    case Size32x32:
        fmt::print("Bg{} 32x32\n", id);
        ReadMap(0);
        break;
    case Size64x32:
        fmt::print("Bg{} 64x32\n", id);
        ReadHorizontalMap(0);
        break;
    case Size32x64:
        fmt::print("Bg{} 32x64\n", id);
        ReadMap(0);
        ReadMap(1);
        break;
    case Size64x64:
        fmt::print("Bg{} 64x64\n", id);
        ReadHorizontalMap(0);
        ReadHorizontalMap(2);
        break;
    default:
        break;
    }

    return all_tiles;
}

void Bg::DumpBg() const {
    if (!Enabled()) {
        return;
    }

    const std::vector<BgTile> all_tiles = ReadEntireTileMap();

    std::vector<u16> bg_buffer(all_tiles.size() * 64);

    const int horizontal_tiles = (ScreenSize() & 0x1) ? 64 : 32;
    const int pixel_width = horizontal_tiles * 8;

    const int vertical_tiles = (ScreenSize() & 0x2) ? 64 : 32;
    const int pixel_height = vertical_tiles * 8;

    int tile_index = 0;
    int scanline_index = 0;
    int vertical_index = 0;
    while (vertical_index < pixel_height) {
        scanline_index = 0;
        tile_index = 0;
        while (scanline_index < pixel_width) {
            const auto& tile = all_tiles[(vertical_index / 8) * horizontal_tiles + tile_index++];
            const int pixel_row = (vertical_index) % 8;
            const int flip_row = tile.v_flip ? (7 - pixel_row) : pixel_row;

            std::array<u16, 8> pixel_colours = lcd.GetTilePixels(tile.tile_addr, SinglePalette(), tile.h_flip,
                                                                 flip_row, tile.palette, 0);

            DrawOverlay(pixel_colours, scanline_index, vertical_index, pixel_width, pixel_height);

            for (int i = 0; i < 8; ++i) {
                bg_buffer[vertical_index * pixel_width + scanline_index++] = pixel_colours[i];
            }
        }

        vertical_index += 1;
    }

    Common::WriteImageToFile(Common::BGR5ToRGB8(bg_buffer), fmt::format("bg{}", id), pixel_width, pixel_height);
}

void Bg::DrawOverlay(std::array<u16, 8>& pixel_colours, int scanline_index, int vertical_index,
                     int pixel_width, int pixel_height) const {
    const int left_edge = scroll_x % pixel_width;
    const int right_edge = (scroll_x + Lcd::h_pixels) % pixel_width;
    const int top_edge = scroll_y % pixel_height;
    const int bottom_edge = (scroll_y + Lcd::v_pixels) % pixel_height;
    std::array<int, 3> channels;
    for (int i = 0; i < 8; ++i) {
        // Determine if this pixel is currently within the viewport.
        bool within_h = false;
        if (right_edge >= left_edge) {
            within_h = scanline_index >= left_edge && scanline_index < right_edge;
        } else {
            within_h = scanline_index >= left_edge || scanline_index < right_edge;
        }

        bool within_v = false;
        if (bottom_edge >= top_edge) {
            within_v = vertical_index >= top_edge && vertical_index < bottom_edge;
        } else {
            within_v = vertical_index >= top_edge || vertical_index < bottom_edge;
        }

        double intensity = 0.1;
        // If this pixel is just outside the edge of the viewport, increase the intensity to draw a border.
        if (vertical_index == (top_edge - 1) % pixel_height || vertical_index == bottom_edge) {
            intensity = 0.3;
            within_v = true;
        }
        if (scanline_index == (left_edge - 1) % pixel_width || scanline_index == right_edge) {
            intensity = 0.3;
            within_h = true;
        }

        if (within_h && within_v) {
            for (int j = 0; j < 3; ++j) {
                channels[j] = (pixel_colours[i] >> (5 * j)) & 0x1F;
                channels[j] += (31.0 - channels[j]) * intensity;
            }
            pixel_colours[i] = (channels[2] << 10) | (channels[1] << 5) | channels[0];
        }

        scanline_index += 1;
    }
}

void Lcd::DumpTileset(int base, bool single_palette) const {
    std::vector<u16> tileset_buffer(1024 * 64);

    const int tile_bytes = single_palette ? 64 : 32;

    const int horizontal_tiles = 32;
    const int pixel_width = horizontal_tiles * 8;

    const int vertical_tiles = 32;
    const int pixel_height = vertical_tiles * 8;

    int tile_index = 0;
    int scanline_index = 0;
    int vertical_index = 0;
    while (vertical_index < pixel_height) {
        scanline_index = 0;
        tile_index = 0;
        while (scanline_index < pixel_width) {
            const int tile_addr = base + ((vertical_index / 8) * horizontal_tiles + tile_index) * tile_bytes;
            const int pixel_row = (vertical_index) % 8;
            tile_index += 1;

            std::array<u16, 8> pixel_colours;
            if (single_palette) {
                pixel_colours = GetTilePixels(tile_addr, single_palette, false, pixel_row, 0, 0);
            } else {
                // Each tile byte specifies the 4-bit palette indices for two pixels.
                for (int i = 0; i < 8; ++i) {
                    const int pixel_addr = tile_addr + pixel_row * 4 + i / 2;
                    const int hi_shift = 8 * (pixel_addr & 0x1);

                    // The lower 4 bits are the palette index for even pixels, and the upper 4 bits are for odd pixels.
                    const int odd_shift = 4 * (i & 0x1);
                    // Shift the palette entry left by 1 so it fills the 5 bits needed by the colour channels.
                    const u8 palette_entry = ((vram[pixel_addr / 2] >> (hi_shift + odd_shift)) & 0xF) << 1;
                    if (palette_entry == 0) {
                        // Palette entry 0 is transparent.
                        pixel_colours[i] = alpha_bit;
                    } else {
                        pixel_colours[i] = (palette_entry << 10) | (palette_entry << 5) | palette_entry;
                    }
                }
            }

            for (int i = 0; i < 8; ++i) {
                tileset_buffer[vertical_index * pixel_width + scanline_index++] = pixel_colours[i];
            }
        }

        vertical_index += 1;
    }

    Common::WriteImageToFile(Common::BGR5ToRGB8(tileset_buffer),
                             fmt::format("tileset{}_{}bit", base / (32 * 1024), (single_palette ? 8 : 4)),
                             pixel_width, pixel_height);
}

} // End namespace Gba
