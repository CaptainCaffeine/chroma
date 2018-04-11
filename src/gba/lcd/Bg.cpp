// This file is a part of Chroma.
// Copyright (C) 2018 Matthew Murray
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

#include "gba/lcd/Bg.h"
#include "gba/lcd/Lcd.h"

namespace Gba {

bool Bg::Enabled() const {
    return (lcd.control & (0x100 << id)) && enable_delay == 0;
}

void Bg::ReadTileMapRow() {
    int row_num = (scroll_y + lcd.vcount) / 8;
    if (ScreenSize() < 2) {
        row_num %= 32;
    } else {
        row_num %= 64;
    }

    if (row_num == previous_row_num && !dirty) {
        // Reuse the previously fetched tilemap & tile data.
        return;
    }

    previous_row_num = row_num;
    dirty = false;
    tiles.clear();

    // Get a row of map entries from the specified screenblock.
    auto ReadRowMap = [this, row_num = row_num % 32](int screenblock) {
        int map_addr = (MapBase() + row_num * 64 + 0x800 * screenblock) / 2;

        for (int i = map_addr; i < map_addr + 32; ++i) {
            tiles.emplace_back(lcd.vram[i]);
        }
    };

    // Build the tilemap info for this scanline from the necessary screenblocks.
    switch (ScreenSize()) {
    case Size32x32:
        ReadRowMap(0);
        break;
    case Size64x32:
        ReadRowMap(0);
        ReadRowMap(1);
        break;
    case Size32x64:
        if (row_num < 32) {
            ReadRowMap(0);
        } else {
            ReadRowMap(1);
        }
        break;
    case Size64x64:
        if (row_num < 32) {
            ReadRowMap(0);
            ReadRowMap(1);
        } else {
            ReadRowMap(2);
            ReadRowMap(3);
        }
        break;
    default:
        break;
    }
}

void Bg::ReadTileData(std::vector<BgTile>& input_tiles) const {
    // Get tile data. Each tile is 32 bytes in 16 palette mode, and 64 bytes in single palette mode.
    const int tile_bytes = SinglePalette() ? 64 : 32;
    for (auto& tile : input_tiles) {
        const int tile_addr = TileBase() + tile.num * tile_bytes;
        if (tile_addr < Lcd::sprite_tile_base) {
            for (int i = 0; i < tile_bytes; i += 2) {
                tile.data[i] = lcd.vram[(tile_addr + i) / 2];
                tile.data[i + 1] = lcd.vram[(tile_addr + i) / 2] >> 8;
            }
        } else {
            // Tiles in OBJ VRAM cannot be used for backgrounds.
            std::fill_n(tile.data.begin(), tile_bytes, 0x8000);
            continue;
        }
    }
}

void Bg::DrawRegularScanline() {
    if (Mosaic() && lcd.vcount % lcd.MosaicBgV() != 0) {
        // Reuse the previous scanline.
        return;
    }

    ReadTileMapRow();
    ReadTileData(tiles);

    const int pixel_row = (scroll_y + lcd.vcount) % 8;

    const int horizontal_tiles = (ScreenSize() & 0x1) ? 64 : 32;
    int tile_index = (scroll_x / 8) % horizontal_tiles;
    int start_offset = scroll_x % 8;

    int scanline_index = 0;
    while (scanline_index < Lcd::h_pixels) {
        const auto& tile = tiles[tile_index];
        tile_index = (tile_index + 1) % horizontal_tiles;
        const int flip_row = tile.v_flip ? (7 - pixel_row) : pixel_row;

        const std::array<u16, 8> pixel_colours = lcd.GetTilePixels(tile.data, SinglePalette(), tile.h_flip,
                                                                   flip_row, tile.palette, 0);

        // The first and last tiles may be partially scrolled off-screen.
        const int end_offset = std::min(Lcd::h_pixels - scanline_index, 8);
        for (int i = start_offset; i < end_offset; ++i) {
            if (Mosaic() && scanline_index % lcd.MosaicBgH() != 0) {
                scanline[scanline_index] = scanline[scanline_index - (scanline_index % lcd.MosaicBgH())];
            } else {
                scanline[scanline_index] = pixel_colours[i];
            }

            scanline_index += 1;
        }
        start_offset = 0;
    }
}

void Bg::DrawAffineScanline() {
    // Affine parameters.
    const int pa = SignExtend<u32>(affine_a, 16);
    const int pb = SignExtend<u32>(affine_b, 16);
    const int pc = SignExtend<u32>(affine_c, 16);
    const int pd = SignExtend<u32>(affine_d, 16);

    const int bg_tile_width = 16 << ScreenSize();
    const int bg_pixel_width = bg_tile_width * 8;

    int scanline_index = 0;
    while (scanline_index < Lcd::h_pixels) {
        int tex_x = (pa * scanline_index + ref_point_x) >> 8;
        int tex_y = (pc * scanline_index + ref_point_y) >> 8;

        if (tex_x >= bg_pixel_width || tex_x < 0 || tex_y >= bg_pixel_width || tex_y < 0) {
            if (WrapAround()) {
                tex_x %= bg_pixel_width;
                tex_y %= bg_pixel_width;

                if (tex_x < 0) {
                    tex_x += bg_pixel_width;
                }
                if (tex_y < 0) {
                    tex_y += bg_pixel_width;
                }
            } else {
                // Out-of-bounds texels are transparent.
                scanline[scanline_index++] = Lcd::alpha_bit;
                continue;
            }
        }

        const int tile_row = tex_y / 8;
        const int tile_index = tile_row * bg_tile_width + tex_x / 8;

        const int map_addr = MapBase() + tile_index;
        int hi_shift = 8 * (map_addr & 0x1);

        const u8 tile_num = (lcd.vram[map_addr / 2] >> hi_shift) & 0xFF;

        // Affine backgrounds can only use single-palette mode.
        constexpr int tile_bytes = 64;
        const int tile_addr = TileBase() + tile_num * tile_bytes;

        const int pixel_row = tex_y % 8;
        const int pixel_addr = tile_addr + pixel_row * 8 + tex_x % 8;
        hi_shift = 8 * (pixel_addr & 0x1);

        const u8 palette_entry = (lcd.vram[pixel_addr / 2] >> hi_shift) & 0xFF;
        if (palette_entry == 0) {
            // Palette entry 0 is transparent.
            scanline[scanline_index] = Lcd::alpha_bit;
        } else {
            scanline[scanline_index] = lcd.pram[palette_entry] & 0x7FFF;
        }

        scanline_index += 1;
    }

    ref_point_x += pb;
    ref_point_y += pd;
}

void Bg::DrawBitmapScanline(int bg_mode, int base_addr) {
    // Affine parameters.
    const int pa = SignExtend<u32>(affine_a, 16);
    const int pb = SignExtend<u32>(affine_b, 16);
    const int pc = SignExtend<u32>(affine_c, 16);
    const int pd = SignExtend<u32>(affine_d, 16);

    int scanline_index = 0;
    while (scanline_index < Lcd::h_pixels) {
        int tex_x = ((pa * scanline_index + ref_point_x) >> 8);
        int tex_y = ((pc * scanline_index + ref_point_y) >> 8);

        if (tex_x >= Lcd::h_pixels || tex_x < 0 || tex_y >= Lcd::v_pixels || tex_y < 0) {
            // Out-of-bounds texels are transparent.
            scanline[scanline_index++] = Lcd::alpha_bit;
            continue;
        }

        if (bg_mode == 3) {
            scanline[scanline_index++] = lcd.vram[tex_y * Lcd::h_pixels + tex_x] & 0x7FFF;
        } else if (bg_mode == 4) {
            // The lower byte is the palette index for even pixels, and the upper byte is for odd pixels.
            const int pixel_addr = base_addr + tex_y * Lcd::h_pixels + tex_x;
            const int odd_shift = 8 * (pixel_addr & 0x1);
            const u8 palette_entry = lcd.vram[pixel_addr / 2] >> odd_shift;
            if (palette_entry == 0) {
                // Palette entry 0 is transparent.
                scanline[scanline_index++] = Lcd::alpha_bit;
            } else {
                scanline[scanline_index++] = lcd.pram[palette_entry] & 0x7FFF;
            }
        } else if (bg_mode == 5) {
            if (tex_x >= 160 || tex_x < 0 || tex_y >= 128 || tex_y < 0) {
                scanline[scanline_index++] = Lcd::alpha_bit;
            } else {
                scanline[scanline_index++] = lcd.vram[base_addr + tex_y * 160 + tex_x] & 0x7FFF;
            }
        }
    }

    ref_point_x += pb;
    ref_point_y += pd;
}

} // End namespace Gba
