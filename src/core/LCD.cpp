// This file is a part of Chroma.
// Copyright (C) 2016 Matthew Murray
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

#include "core/LCD.h"

namespace Core {

LCD::LCD(Memory& memory) : mem(memory) { }

void LCD::UpdateLCD() {
    // Check if the LCD has been set on or off.
    UpdatePowerOnState();

    if (!lcd_on) {
        return;
    }

    // DMG ONLY FOR NOW -- cgb timings also include dmg mode, single speed, and double speed.

    scanline_cycles += 4;
    stat_interrupt_signal = false;

    UpdateLY();

    UpdateLYCompareSignal();

    if (mem.ly == 0) {
        // We stay in mode 1 for all of scanline 153, even though LY reads 0. Maybe I should keep a separate scanline
        // variable? Depends if it would be useful for CGB modes.
        if (STATMode() != 1) {
            if (scanline_cycles == 4) {
                SetSTATMode(2);
                stat_interrupt_signal |= Mode2CheckEnabled();
            } else if (scanline_cycles == 84) {
                SetSTATMode(3);
                RenderScanline();
            } else if (scanline_cycles == 256) {
                // Inaccurate. The duration of Mode 3 varies depending on the number of sprites drawn
                // for the scanline, but I don't know by how much. I also don't know the exact timings of the Mode 0
                // interrupt, so I'm assuming it's triggered as soon as Mode 0 is entered.
                SetSTATMode(0);
                stat_interrupt_signal |= Mode0CheckEnabled();
            }
        }
    } else if (mem.ly <= 143) {
        if (scanline_cycles == 0) {
            // AntonioND claims the Mode 2 STAT interrupt happens the cycle before Mode 2 is entered?
            // Except for scanline 0.
            stat_interrupt_signal |= Mode2CheckEnabled();
        } else if (scanline_cycles == 4) {
            SetSTATMode(2);
        } else if (scanline_cycles == 84) {
            SetSTATMode(3);
            RenderScanline();
        } else if (scanline_cycles == 256) {
            // Inaccurate. The duration of Mode 3 varies depending on the number of sprites drawn
            // for the scanline, but I don't know by how much. I also don't know the exact timings of the Mode 0
            // interrupt, so I'm assuming it's triggered as soon as Mode 0 is entered.
            SetSTATMode(0);
            stat_interrupt_signal |= Mode0CheckEnabled();
        }
    } else if (mem.ly == 144) {
        if (scanline_cycles == 4) {
            mem.RequestInterrupt(Interrupt::VBLANK);
            SetSTATMode(1);
            // The OAM STAT interrupt is also triggered on entering Mode 1.
            stat_interrupt_signal |= Mode2CheckEnabled();
        }
    }

    // Disabling and reenabling the Mode 1 check during VBLANK will cause the STAT interrupt to trigger again.
    stat_interrupt_signal |= (Mode1CheckEnabled() && STATMode() == 1);

    // The STAT interrupt is triggered on a rising edge of the STAT interrupt signal, which is a 4 way logical OR
    // between each STAT check. As a result, if two events which would have triggered a STAT interrupt happen on 
    // consecutive machine cycles, the second one will not cause an interrupt to be requested.
    if (stat_interrupt_signal && !prev_interrupt_signal) {
        mem.RequestInterrupt(Interrupt::STAT);
    }
    prev_interrupt_signal = stat_interrupt_signal;

//    // Mode timings debug
//    if (scanline_cycles == 0) {
//        std::cout << "\n LY=" << std::setw(3) << std::setfill(' ') << static_cast<unsigned int>(mem.ly) << " ";
//    }
//    std::cout << STATMode() << " ";
}

void LCD::UpdatePowerOnState() {
    u8 lcdc_power_on = mem.lcdc & 0x80;
    if (lcdc_power_on ^ lcd_on) {
        lcd_on = lcdc_power_on;

        if (lcd_on) {
            // Initialize scanline cycle count (to 452 instead of 0, so it ticks over to 0 in UpdateLY()).
            scanline_cycles = 452;
        } else {
            // LY register is fixed to 0x00 when the LCD is off.
            mem.ly = 0;
            // LCD behaves as if it's in Mode 1 when it is off. I don't know if this is what the STAT register 
            // actually reads though...
            SetSTATMode(1);
        }
    }
}

void LCD::UpdateLY() {
    if (mem.ly == 153) {
        // LY is 153 only for one machine cycle at the beginning of scanline 153, then wraps back to 0.
        mem.ly = 0;
    } else if (scanline_cycles == 456) {
        // Reset scanline cycle counter.
        scanline_cycles = 0;

        // LY does not increase at the end of scanline 153, it stays 0 until the end of scanline 0.
        // Otherwise, increment LY.
        if (mem.ly == 0 && STATMode() == 1) {
            SetSTATMode(0);
            stat_interrupt_signal |= Mode0CheckEnabled();
        } else {
            ++mem.ly;
        }
    }
}

// Handles setting the LYC=LY compare bit and corresponding STAT interrupt on DMG. When LY changes, the LY=LYC bit is 
// set to zero that machine cycle, then on the next machine cycle, LY=LYC is set using the new LY value and the STAT
// interrupt can be fired. This will not be interrupted even if LY changes again on the second cycle (which happens 
// on scanline 153). In that case, the two events caused by an LY change begin on the following cycle.
void LCD::UpdateLYCompareSignal() {
    if (LY_compare_equal_forced_zero) {
        SetLYCompare(mem.ly_compare == LY_last_cycle);

        stat_interrupt_signal |= (LYCompareCheckEnabled() && LYCompareEqual());

        // Don't update LY_last_cycle.
        LY_compare_equal_forced_zero = false;
    } else if (mem.ly != LY_last_cycle) {
        SetLYCompare(false);
        LY_compare_equal_forced_zero = true;
        LY_last_cycle = mem.ly;
    } else {
        SetLYCompare(mem.ly_compare == mem.ly);
        LY_last_cycle = mem.ly;
    }
}

void LCD::RenderScanline() {
    if (BGEnabled()) {
        // The background is composed of 32x32 tiles of 8x8 pixels. The scroll registers (SCY and SCX) allow the
        // top-left corner of the screen to be positioned anywhere on the background, and the background wraps around
        // when it hits the edge.
        // The background tile map is located at either 0x9800-0x9BFF or 0x9C00-0x9FFF, and consists of 32 rows
        // of 32 bytes each to index the background tiles. We first determine which row we need to fetch from the
        // current values of SCY and LY.
        unsigned int row_num = ((mem.scroll_y + mem.ly) / 8) % num_tiles;
        u16 tile_map_addr = BGTileMapStartAddr() + row_num * num_tiles;

        // Get the row of tile indicies from VRAM.
        mem.CopyFromVRAM(tile_map_addr, num_tiles, row_tile_map.begin());

        // The background tiles are located at either 0x8000-0x8FFF or 0x8800-0x97FF. For the first region, the
        // tile map indicies are unsigned offsets from 0x8000; for the second region, the indicies are signed
        // offsets from 0x9000.
        // Fetch the tile bitmaps pointed to by the indicies in the tile map row. Each tile is 16 bytes.
        if (TileDataStartAddr() == 0x9000) {
            // Tile map indexes are signed, with 0 at 0x9000.
            std::copy(row_tile_map.cbegin(), row_tile_map.cend(), signed_row_tile_map.begin());
            FetchTiles(signed_row_tile_map);
        } else {
            // Tile map indexes are unsigned, with 0 at 0x8000.
            FetchTiles(row_tile_map);
        }

        // Determine which row of pixels we're on.
        unsigned int tile_row = (mem.scroll_y + mem.ly) % 8;
        // Determine in which tile we start reading data.
        unsigned int start_tile = mem.scroll_x / 8;

        // Each row of 8 pixels in a tile is 2 bytes. The first byte contains the low bit of the palette index for
        // each pixel, and the second byte contains the high bit of the palette index. The background palette in
        // DMG mode is in the BGP register at 0xFF47.

        // Decode the tile data and determine the pixel colors.
        std::size_t tile_data_index = tile_row * 2 + start_tile * 16;

        auto row_pixel = row_pixels.begin();
        while (row_pixel != row_pixels.end()) {
            // Get the two bytes describing the row of the current tile.
            u8 lsb = tile_data[tile_data_index];
            u8 msb = tile_data[tile_data_index + 1];

            // Combine the bytes together to get the palette indicies.
            std::array<unsigned int, 8> pixel_indicies;
            for (std::size_t j = 0; j < 7; ++j) {
                pixel_indicies[j] = ((lsb >> (7-j)) & 0x01) | ((msb >> (6-j)) & 0x02);
            }
            pixel_indicies[7] = (lsb & 0x01) | ((msb << 1) & 0x02); // Can't shift right by -1...

            // Get the colours corresponding to the palette indicies.
            for (auto plt_index : pixel_indicies) {
                *row_pixel++ = shades[(mem.bg_palette >> (plt_index * 2)) & 0x03];
            }

            // Increment the tile index to the next tile, and wrap around if we hit the end.
            tile_data_index = (tile_data_index + 16) % tile_data.size();
        }

        // The `row_pixels` buffer is 22 full tiles wide and contains 176 pixels.
        // We determine which 160 pixels get transferred to the framebuffer from the SCX register.
        auto row_start_iter = row_pixels.begin() + (mem.scroll_x % 8);
        std::copy(row_start_iter, row_start_iter + 160, framebuffer.begin() + mem.ly * 160);
    } else if (WindowEnabled()) {
        // The Window tile map is located at either 0x9800-0x9BFF or 0x9C00-0x9FFF.
    } else if (SpritesEnabled()) {
    }
}

} // End namespace Core
