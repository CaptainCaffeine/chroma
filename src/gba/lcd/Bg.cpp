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

void Bg::GetRowMapInfo() {
    tiles.clear();

    int row_num = (scroll_y + lcd.vcount) / 8;
    if (ScreenSize<int>() < 2) {
        row_num %= 32;
    } else {
        row_num %= 64;
    }

    // Get a row of map entries from the specified screenblock.
    auto ReadRowMap = [this, row_num](int screenblock) {
        int map_addr = (MapBase() + row_num * 64 + 0x800 * screenblock) / 2;

        for (int i = map_addr; i < map_addr + 32; ++i) {
            tiles.emplace_back(lcd.vram[i]);
        }
    };

    // Build the tilemap info for this scanline from the necessary screenblocks.
    switch (ScreenSize<Regular>()) {
    case Regular::Size32x32:
        ReadRowMap(0);
        break;
    case Regular::Size64x32:
        ReadRowMap(0);
        ReadRowMap(1);
        break;
    case Regular::Size32x64:
        if (row_num < 32) {
            ReadRowMap(0);
        } else {
            ReadRowMap(1);
        }
        break;
    case Regular::Size64x64:
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

void Bg::GetTileData() {
    // Get tile data. Each tile is 32 bytes in 16 palette mode, and 64 bytes in single palette mode.
    const int tile_bytes = SinglePalette() ? 64 : 32;
    for (auto& tile : tiles) {
        int tile_addr = (TileBase() + tile.num * tile_bytes) / 2;
        for (int i = 0; i < tile_bytes; i += 2) {
            tile.data[i] = lcd.vram[tile_addr + i / 2];
            tile.data[i + 1] = lcd.vram[tile_addr + i / 2] >> 8;
        }
    }
}

void Bg::DrawScanline() {
    const int tile_row = (scroll_y + lcd.vcount) % 8;

    const int horizontal_tiles = (ScreenSize<int>() & 0x1) ? 64 : 32;
    int tile_index = (scroll_x / 8) % horizontal_tiles;
    int start_offset = scroll_x % 8;

    int scanline_index = 0;
    while (scanline_index < Lcd::h_pixels) {
        auto& tile = tiles[tile_index];
        tile_index = (tile_index + 1) % horizontal_tiles;
        const int flip_row = tile.v_flip ? (8 - tile_row) : tile_row;

        std::array<u16, 8> pixel_colours;
        if (SinglePalette()) {
            // Each tile byte specifies the 8-bit palette index for a pixel.
            for (int i = 0; i < 8; ++i) {
                u8 palette_entry = tile.data[flip_row * 8 + i];
                pixel_colours[i] = lcd.pram[palette_entry];

                if (palette_entry == 0) {
                    // Palette entry 0 is transparent.
                    pixel_colours[i] |= Lcd::alpha_bit;
                }
            }
        } else {
            // Each tile byte specifies the 4-bit palette indices for two pixels.
            for (int i = 0; i < 8; ++i) {
                // The lower 4 bits are the palette index for even pixels, and the upper 4 bits are for odd pixels.
                const int odd_shift = 4 * (i & 0x1);
                u8 palette_entry = (tile.data[flip_row * 4 + i / 2] >> odd_shift) & 0xF;
                pixel_colours[i] = lcd.pram[tile.palette * 16 + palette_entry];

                if (palette_entry == 0) {
                    // Palette entry 0 is transparent.
                    pixel_colours[i] |= Lcd::alpha_bit;
                }
            }
        }

        if (tile.h_flip) {
            std::reverse(pixel_colours.begin(), pixel_colours.end());
        }

        // The first and last tiles may be partially scrolled off-screen.
        const int end_offset = std::min(Lcd::h_pixels - scanline_index, 8);
        for (int i = start_offset; i < end_offset; ++i) {
            scanline[scanline_index++] = pixel_colours[i];
        }
        start_offset = 0;
    }
}

} // End namespace Gba
