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
#include "core/memory/Memory.h"
#include "core/GameBoy.h"

namespace Core {

LCD::LCD() : back_buffer(160*144) {}

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

    if (ly == 0) {
        if (STATMode() != 1) {
            if (scanline_cycles == 4) {
                SetSTATMode(2);
            } else if (scanline_cycles == 84) {
                SetSTATMode(3);
                RenderScanline();
            } else if (scanline_cycles == Mode3Cycles()) {
                // Inaccurate. The duration of Mode 3 varies depending on the number of sprites drawn
                // for the scanline, but I don't know by how much.
                SetSTATMode(0);
            }
        }
    } else if (ly <= 143) {
        // AntonioND claims that except for scanline 0, the Mode 2 STAT interrupt happens the cycle before Mode 2
        // is entered. However, doing this causes most of Mooneye-GB's STAT timing tests to fail.
        if (scanline_cycles == 4) {
            SetSTATMode(2);
        } else if (scanline_cycles == 84) {
            SetSTATMode(3);
            RenderScanline();
        } else if (scanline_cycles == Mode3Cycles()) {
            // Inaccurate. The duration of Mode 3 varies depending on the number of sprites drawn
            // for the scanline, but I don't know by how much. I also don't know the exact timings of the Mode 0
            // interrupt, so I'm assuming it's triggered as soon as Mode 0 is entered.
            SetSTATMode(0);
        }
    } else if (ly == 144) {
        if (scanline_cycles == 4) {
            mem->RequestInterrupt(Interrupt::VBLANK);
            SetSTATMode(1);
            // The OAM STAT interrupt is also triggered on entering Mode 1.
            stat_interrupt_signal |= Mode2CheckEnabled();

            // Swap front and back buffers now that we've completed a frame.
            gameboy->SwapBuffers(back_buffer);
        }
    }

    CheckSTATInterruptSignal();

//    // Mode timings debug
//    if (scanline_cycles == 0) {
//        std::cout << "\n LY=" << std::setw(3) << std::setfill(' ') << static_cast<unsigned int>(ly) << " ";
//    }
//    std::cout << STATMode() << " ";
}

void LCD::UpdatePowerOnState() {
    u8 lcdc_power_on = lcdc & 0x80;
    if (lcdc_power_on ^ lcd_on) {
        lcd_on = lcdc_power_on;

        if (lcd_on) {
            // Initialize scanline cycle count (to 452 instead of 0, so it ticks over to 0 in UpdateLY()).
            scanline_cycles = 452;
            SetSTATMode(1);
        } else {
            ly = 0;
            SetSTATMode(0);
            stat_interrupt_signal = 0;
            prev_interrupt_signal = 0;

            // Clear the framebuffer.
            std::fill_n(back_buffer.begin(), 160*144, 0xFFFFFF00);
            gameboy->SwapBuffers(back_buffer);
        }
    }
}

void LCD::UpdateLY() {
    if (ly == 153) {
        // LY is 153 only for one machine cycle at the beginning of scanline 153, then wraps back to 0.
        ly = 0;
    } else if (scanline_cycles == 456) {
        // Reset scanline cycle counter.
        scanline_cycles = 0;

        // LY does not increase at the end of scanline 153, it stays 0 until the end of scanline 0.
        // Otherwise, increment LY.
        if (ly == 0 && STATMode() == 1) {
            SetSTATMode(0); // Does this actually happen? Or do we spend the first cycle in mode 1?

            // Changes to the window Y position register are ignored until next VBLANK.
            window_y_frame_val = window_y;
            window_progress = 0x00;
        } else {
            ++ly;
        }
    }
}

int LCD::Mode3Cycles() const {
    // The cycles taken by mode 3 increase by a number of factors.
    int cycles = 256;

    // Mode 3 cycles increase depending on how much of the first tile is cut off by the current value of SCX.
    int scx_mod = scroll_x % 8;
    if (scx_mod > 0 && scx_mod < 5) {
        cycles += 4;
    } else if (scx_mod > 4) {
        cycles += 8;
    }

    return cycles;
}

// Handles setting the LYC=LY compare bit and corresponding STAT interrupt on DMG. When LY changes, the LY=LYC bit is 
// set to zero that machine cycle, then on the next machine cycle, LY=LYC is set using the new LY value and the STAT
// interrupt can be fired. This will not be interrupted even if LY changes again on the second cycle (which happens 
// on scanline 153). In that case, the two events caused by an LY change begin on the following cycle.
void LCD::UpdateLYCompareSignal() {
    if (LY_compare_equal_forced_zero) {
        SetLYCompare(ly_compare == LY_last_cycle);

        // Don't update LY_last_cycle.
        LY_compare_equal_forced_zero = false;
    } else if (ly != LY_last_cycle) {
        SetLYCompare(false);
        LY_compare_equal_forced_zero = true;
        LY_last_cycle = ly;
    } else {
        SetLYCompare(ly_compare == ly);
        LY_last_cycle = ly;
    }
}

void LCD::CheckSTATInterruptSignal() {
    stat_interrupt_signal |= (Mode0CheckEnabled() && STATMode() == 0);
    stat_interrupt_signal |= (Mode1CheckEnabled() && STATMode() == 1);
    stat_interrupt_signal |= (Mode2CheckEnabled() && STATMode() == 2);
    stat_interrupt_signal |= (LYCompareCheckEnabled() && LYCompareEqual());

    // The STAT interrupt is triggered on a rising edge of the STAT interrupt signal, which is a 4 way logical OR
    // between each STAT check. As a result, if two events which would have triggered a STAT interrupt happen on
    // consecutive machine cycles, the second one will not cause an interrupt to be requested.
    if (stat_interrupt_signal && !prev_interrupt_signal) {
        mem->RequestInterrupt(Interrupt::STAT);
    }
    prev_interrupt_signal = stat_interrupt_signal;
}

void LCD::RenderScanline() {
    std::size_t num_bg_pixels;
    if (WindowEnabled()) {
        num_bg_pixels = (window_x < 7) ? 0 : window_x - 7;
    } else {
        num_bg_pixels = 160;
    }

    if (BGEnabled()) {
        RenderBackground(num_bg_pixels);
    } else {
        // If disabled, we need to blank what isn't covered by the window.
        std::fill_n(row_buffer.begin(), num_bg_pixels, 0xFFFFFF00);
    }

    if (WindowEnabled()) {
        RenderWindow(num_bg_pixels);
    }

    if (SpritesEnabled()) {
        RenderSprites();
    }

    // Copy the row buffer into the back buffer. The last 8 pixels of the row buffer are extra off-the-end space
    // to simplify the background & window rendering code, and so they are discarded.
    std::copy(row_buffer.begin(), row_buffer.end() - 8, back_buffer.begin() + ly * 160);
}

void LCD::RenderBackground(std::size_t num_bg_pixels) {
    // The background is composed of 32x32 tiles of 8x8 pixels. The scroll registers (SCY and SCX) allow the
    // top-left corner of the screen to be positioned anywhere on the background, and the background wraps around
    // when it hits the edge.

    // The background tile map is located at either 0x9800-0x9BFF or 0x9C00-0x9FFF, and consists of 32 rows
    // of 32 bytes each to index the background tiles. We first determine which row we need to fetch from the
    // current values of SCY and LY.
    unsigned int row_num = ((scroll_y + ly) / 8) % num_tiles;
    u16 tile_map_addr = BGTileMapStartAddr() + row_num * tile_map_row_bytes;

    // Get the row of tile indicies from VRAM.
    mem->CopyFromVRAM(tile_map_addr, tile_map_row_bytes, row_tile_map.begin());

    // The background tiles are located at either 0x8000-0x8FFF or 0x8800-0x97FF. For the first region, the
    // tile map indicies are unsigned offsets from 0x8000; for the second region, the indicies are signed
    // offsets from 0x9000.

    // Fetch the tile bitmaps pointed to by the indicies in the tile map row. Each tile is 16 bytes.
    if (TileDataStartAddr() == 0x9000) {
        std::copy(row_tile_map.cbegin(), row_tile_map.cend(), signed_row_tile_map.begin());
        FetchTiles(signed_row_tile_map);
    } else {
        FetchTiles(row_tile_map);
    }

    // Each row of 8 pixels in a tile is 2 bytes. The first byte contains the low bit of the palette index for
    // each pixel, and the second byte contains the high bit of the palette index. The background palette in
    // DMG mode is in the BGP register at 0xFF47.

    // Determine which row of pixels we're on, and in which tile we start reading data.
    unsigned int tile_row = (scroll_y + ly) % 8;
    unsigned int start_tile = scroll_x / 8;
    // Two bytes per tile row, 16 bytes per tile.
    std::size_t tile_data_index = tile_row * 2 + start_tile * 16;

    // If necessary, throw away the first few pixels of the first tile, based on SCX.
    std::size_t row_pixel = RenderFirstTile(0, tile_data_index, scroll_x % 8);

    // Increment the tile index to the next tile, and wrap around if we hit the end.
    tile_data_index = (tile_data_index + 16) % tile_data.size();

    while (row_pixel < num_bg_pixels) {
        // Get the two bytes describing the row of the current tile.
        u8 lsb = tile_data[tile_data_index];
        u8 msb = tile_data[tile_data_index + 1];

        DecodePixelColoursFromPalette(lsb, msb, bg_palette, false);

        for (const auto& pixel_colour : pixel_colours) {
            row_buffer[row_pixel++] = pixel_colour;
        }

        tile_data_index = (tile_data_index + 16) % tile_data.size();
    }
}

void LCD::RenderWindow(std::size_t num_bg_pixels) {
    // The window is composed of 32x32 tiles of 8x8 pixels (of which only 21x18 tiles can be seen). Unlike the
    // background, the window cannot be scrolled; it is always displayed from its top-left corner and does not
    // wrap around. Instead, the position of its top-left corner can be set with the WY and WX registers.

    // The behaviour of WY differs from SCY. At the beginning of a frame, the LCD hardware stores the current value
    // of WY and ignores all writes to that register until the next VBLANK. In addition, the row of window pixels
    // rendered to a scanline is not directly dependent on LY. While the window is enabled, the rows of the window
    // are rendered one after another starting at the top of the window. However, if it is disabled during HBLANK
    // and later re-enabled before the frame has ended, the window will resume drawing from the exact row
    // at which it left off; ignoring the LY increments that happened while it was disabled.

    // The window tile map is located at either 0x9800-0x9BFF or 0x9C00-0x9FFF, and consists of 32 rows
    // of 32 bytes each to index the window tiles. We first determine which row we need to fetch from the
    // current internal value of the window progression.
    u16 tile_map_addr = WindowTileMapStartAddr() + (window_progress / 8) * tile_map_row_bytes;

    // Get the row of tile indicies from VRAM.
    mem->CopyFromVRAM(tile_map_addr, tile_map_row_bytes, row_tile_map.begin());

    // The window tiles are located at either 0x8000-0x8FFF or 0x8800-0x97FF. For the first region, the
    // tile map indicies are unsigned offsets from 0x8000; for the second region, the indicies are signed
    // offsets from 0x9000.

    // Fetch the tile bitmaps pointed to by the indicies in the tile map row. Each tile is 16 bytes.
    if (TileDataStartAddr() == 0x9000) {
        std::copy(row_tile_map.cbegin(), row_tile_map.cend(), signed_row_tile_map.begin());
        FetchTiles(signed_row_tile_map);
    } else {
        FetchTiles(row_tile_map);
    }

    // Each row of 8 pixels in a tile is 2 bytes. The first byte contains the low bit of the palette index for
    // each pixel, and the second byte contains the high bit of the palette index. The window palette in
    // DMG mode is in the BGP register at 0xFF47.

    // Determine which row of pixels we're on.
    unsigned int tile_row = (window_progress) % 8;
    // Two bytes per tile row. The window always starts rendering at the leftmost/first tile.
    std::size_t tile_data_index = tile_row * 2;

    // If necessary, throw away the first few pixels of the first tile, based on WX.
    std::size_t row_pixel = RenderFirstTile(num_bg_pixels, tile_data_index, (window_x < 7) ? 7 - window_x : 0);

    // Increment the tile index to the next tile.
    tile_data_index += 16;

    while (row_pixel < 160) {
        // Get the two bytes describing the row of the current tile.
        u8 lsb = tile_data[tile_data_index];
        u8 msb = tile_data[tile_data_index + 1];

        DecodePixelColoursFromPalette(lsb, msb, bg_palette, false);

        for (const auto& pixel_colour : pixel_colours) {
            row_buffer[row_pixel++] = pixel_colour;
        }

        tile_data_index += 16;
    }

    // Increment internal window progression.
    ++window_progress;
}

std::size_t LCD::RenderFirstTile(std::size_t start_pixel, std::size_t tile_data_index, std::size_t throwaway) {
    // Get the two bytes describing the row of the current tile.
    u8 lsb = tile_data[tile_data_index];
    u8 msb = tile_data[tile_data_index + 1];

    DecodePixelColoursFromPalette(lsb, msb, bg_palette, false);

    // Throw away the first pixels of the tile.
    for (std::size_t pixel = throwaway; pixel < 8; ++pixel) {
        row_buffer[start_pixel++] = pixel_colours[pixel];
    }

    // Return the number of pixels written to the row buffer.
    return start_pixel;
}

void LCD::RenderSprites() {
    mem->CopyOAM(oam_ram.begin());
    std::deque<SpriteAttrs> oam_sprites;

    // The sprite_gap is the distance between the bottom of the sprite and its Y position (8 for 8x8, 0 for 8x16).
    unsigned int sprite_gap = SpriteSize() % 16;
    // The tile index mask is 0xFF for 8x8, 0xFE for 8x16.
    u8 index_mask = (sprite_gap >> 3) | 0xFE;

    // Store the first 10 sprites from OAM which are on this scanline.
    for (std::size_t i = 0; i < oam_ram.size(); i += 4) {
        // Check that the sprite is not off the screen.
        if (oam_ram[i] > sprite_gap && oam_ram[i] < 160) {
            // Check that the sprite is on the current scanline.
            if (ly < oam_ram[i] - sprite_gap && static_cast<int>(ly) >= static_cast<int>(oam_ram[i]) - 16) {
                oam_sprites.emplace_front(oam_ram[i], oam_ram[i+1], oam_ram[i+2] & index_mask, oam_ram[i+3]);
            }
        }

        if (oam_sprites.size() == 10) {
            break;
        }
    }

    // Remove all sprites with an off-screen X position.
    oam_sprites.erase(std::remove_if(oam_sprites.begin(), oam_sprites.end(), [](SpriteAttrs sa) {
                          return sa.x_pos >= 168 || sa.x_pos == 0;
                      }),
                      oam_sprites.end());

    // Sprite drawing priority is based on its position in OAM and its X position. Sprite are drawn in descending X
    // order. If two sprites overlap, the one that has a lower position in OAM is drawn on top. oam_sprites already
    // contains the sprites for this line in decreasing OAM position, so we sort them by decreasing X position.
    std::stable_sort(oam_sprites.begin(), oam_sprites.end(), [](SpriteAttrs sa1, SpriteAttrs sa2) {
        return sa2.x_pos < sa1.x_pos;
    });

    FetchSpriteTiles(oam_sprites);

    // Each row of 8 pixels in a tile is 2 bytes. The first byte contains the low bit of the palette index for
    // each pixel, and the second byte contains the high bit of the palette index. The object palettes in
    // DMG mode are in the OBP0 and OBP1 registers at 0xFF48 and 0xFF49.

    for (const SpriteAttrs& sa : oam_sprites) {
        // Determine which row of the sprite tile is being drawn.
        unsigned int tile_row = (ly - (sa.y_pos - 16));

        // If this sprite has the Y flip flag set, get the mirrored row in the other half of the sprite.
        if (sa.attrs & 0x40) {
            tile_row = (SpriteSize() - 1) - tile_row;
        }

        // Two bytes per tile row.
        tile_row *= 2;

        // Get the two bytes containing the row of the tile.
        u8 lsb = sa.sprite_tiles[tile_row];
        u8 msb = sa.sprite_tiles[tile_row + 1];

        if (sa.attrs & 0x10) {
            DecodePixelColoursFromPalette(lsb, msb, obj_palette1, true);
        } else {
            DecodePixelColoursFromPalette(lsb, msb, obj_palette0, true);
        }

        // If this sprite has the X flip flag set, reverse the pixels.
        if (sa.attrs & 0x20) {
            std::reverse(pixel_colours.begin(), pixel_colours.end());
        }

        auto pixel_iter = pixel_colours.cbegin();
        auto pixel_end_iter = pixel_colours.cend();

        // If the sprite's X pos is less than 8 or greater than 159, part of the sprite will be cut off.
        unsigned int draw_start_pos = sa.x_pos - 8;
        if (sa.x_pos < 8) {
            pixel_iter += 8 - sa.x_pos;
            draw_start_pos = 0;
        } else if (sa.x_pos > 160) {
            pixel_end_iter -= (sa.x_pos - 160);
        }

        // Bit 7 in the sprite's OAM attribute byte decides whether or not the sprite will be drawn above or below
        // the background and window. If drawn below, then bg pixels of colour 0x00 are transparent. If drawn above,
        // the sprite pixels of colour 0x00 are transparent.
        auto row_buffer_iter = row_buffer.begin() + draw_start_pos;
        if (sa.attrs & 0x80) {
            // Draw the sprite below the background.
            while (pixel_iter != pixel_end_iter) {
                if ((*pixel_iter & 0xFF) != 0xFF && *row_buffer_iter == shades[bg_palette & 0x03]) {
                    *row_buffer_iter = *pixel_iter;
                }
                ++pixel_iter;
                ++row_buffer_iter;
            }
        } else {
            // Draw the sprite above the background.
            while (pixel_iter != pixel_end_iter) {
                if ((*pixel_iter & 0xFF) != 0xFF) {
                    *row_buffer_iter = *pixel_iter;
                }
                ++pixel_iter;
                ++row_buffer_iter;
            }
        }
    }
}

template<typename T, std::size_t N>
void LCD::FetchTiles(const std::array<T, N>& tile_indicies) {
    u16 region_start_addr = TileDataStartAddr();
    auto tile_data_iter = tile_data.begin();
    // T is either u8 or s8, depending on the current tile data region.
    for (T index : tile_indicies) {
        u16 tile_addr = region_start_addr + index * static_cast<T>(tile_bytes);
        mem->CopyFromVRAM(tile_addr, tile_bytes, tile_data_iter);
        tile_data_iter += tile_bytes;
    }
}

void LCD::FetchSpriteTiles(std::deque<SpriteAttrs>& sprites) {
    std::size_t tile_size = tile_bytes;
    if (SpriteSize() == 16) {
        tile_size *= 2;
    }

    for (SpriteAttrs& sa : sprites) {
        u16 tile_addr = 0x8000 | (static_cast<u16>(sa.tile_index) << 4);
        mem->CopyFromVRAM(tile_addr, tile_size, sa.sprite_tiles.begin());
    }
}

void LCD::DecodePixelColoursFromPalette(u8 lsb, u8 msb, u8 palette, bool sprite) {
    for (std::size_t j = 0; j < 7; ++j) {
        pixel_colours[j] = ((lsb >> (7-j)) & 0x01) | ((msb >> (6-j)) & 0x02);
    }
    pixel_colours[7] = (lsb & 0x01) | ((msb << 1) & 0x02); // Can't shift right by -1...

    for (auto& index : pixel_colours) {
        if (sprite && index == 0) {
            // Palette index 0 is transparent for sprites.
            index = 0xFFFFFFFF;
        } else {
            index = shades[(palette >> (index * 2)) & 0x03];
        }
    }
}

} // End namespace Core
