// This file is a part of Chroma.
// Copyright (C) 2016-2017 Matthew Murray
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

#include "common/Util.h"
#include "core/lcd/LCD.h"
#include "core/memory/Memory.h"

namespace Core {

void LCD::DumpBackBuffer() const {
    Common::WritePPMFile(Common::BGR5ToRGB8(back_buffer), "screenshot.ppm", 160, 144);
}

void LCD::DumpBGWin(u16 start_addr, const std::string& filename) {
    tile_data.clear();
    // Get BG/Window tile map.
    std::size_t tile_map_len = tile_map_row_len * tile_map_row_len;
    std::vector<u8> tile_map(tile_map_len);
    mem->CopyFromVRAM(start_addr, tile_map_len, 0, tile_map.begin());

    if (mem->game_mode == GameMode::DMG) {
        for (std::size_t i = 0; i < tile_map.size(); ++i) {
            tile_data.emplace_back(tile_map[i]);
        }
    } else {
        // Get BG tile attributes.
        std::vector<u8> tile_attrs(tile_map_len);
        mem->CopyFromVRAM(start_addr, tile_map_len, 1, tile_attrs.begin());

        for (std::size_t i = 0; i < tile_map.size(); ++i) {
            tile_data.emplace_back(tile_map[i], tile_attrs[i]);
        }
    }

    FetchTiles();

    std::vector<u16> bg_buffer(256*256);

    std::size_t buffer_pixel = 0;
    for (std::size_t i = 0; i < 32; ++i) {
        for (std::size_t row = 0; row < 8; ++row) {
            auto tile_iter = tile_data.begin() + i * 32;

            for (std::size_t j = 0; j < 32; ++j) {
                std::size_t tile_row = row * 2;
                // If this tile has the Y flip flag set, decode the mirrored row in the other half of the tile.
                DecodePaletteIndices(tile_iter->tile, (tile_iter->y_flip) ? (14 - tile_row) : tile_row);

                // If this tile has the X flip flag set, reverse the pixels.
                if (tile_iter->x_flip) {
                    std::reverse(pixel_colours.begin(), pixel_colours.end());
                }

                if (mem->game_mode == GameMode::DMG) {
                    GetPixelColoursFromPaletteDMG(bg_palette_dmg, false);
                } else {
                    GetPixelColoursFromPaletteCGB(tile_iter->palette_num, false);
                }

                // Copy the pixels to the row buffer.
                for (const auto& pixel_colour : pixel_colours) {
                    bg_buffer[buffer_pixel++] = pixel_colour;
                }

                ++tile_iter;
            }
        }
    }

    Common::WritePPMFile(Common::BGR5ToRGB8(bg_buffer), filename, 256, 256);
}

void LCD::DumpTileSet(int bank) {
    std::vector<u8> tileset(0x17FF);
    mem->CopyFromVRAM(0x8000, 0x1800, bank, tileset.begin());

    // 24 rows of 16 tiles.
    std::vector<u16> buffer(192*128);

    std::array<u8, tile_bytes> tile;
    std::size_t buffer_pixel = 0;

    for (std::size_t i = 0; i < 24; ++i) {

        // Draw the 8 rows (scanlines) of the current tiles.
        for (std::size_t row = 0; row < 8; ++row) {

            std::size_t tile_index = i * tile_bytes * 16;
            for (std::size_t j = 0; j < 16; ++j) {
                std::size_t tile_row = row * 2;
                // If this tile has the Y flip flag set, decode the mirrored row in the other half of the tile.
                std::copy_n(tileset.begin() + tile_index + j * tile_bytes, tile_bytes, tile.begin());
                DecodePaletteIndices(tile, tile_row);

                u8 palette = 0xE4;
                for (auto& colour : pixel_colours) {
                        colour = shades[(palette >> (colour * 2)) & 0x03];
                }

                // Copy the pixels to the row buffer.
                for (const auto& pixel_colour : pixel_colours) {
                    buffer[buffer_pixel++] = pixel_colour;
                }
            }
        }
    }

    Common::WritePPMFile(Common::BGR5ToRGB8(buffer), std::string("tileset") + std::to_string(bank) + ".ppm", 128, 192);
}

void LCD::DumpEverything() {
    DumpBackBuffer();
    DumpBGWin(BGTileMapStartAddr(), "bg_dump.ppm");
    DumpBGWin(WindowTileMapStartAddr(), "win_dump.ppm");
    DumpTileSet(0);
    if (mem->game_mode == GameMode::CGB) {
        DumpTileSet(1);
    }
}

} // End namespace Core
