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

#include "core/lcd/LCD.h"
#include "core/memory/Memory.h"
#include "core/GameBoy.h"

namespace Core {

BGAttrs::BGAttrs(u8 tile_index) : index(tile_index) {}

BGAttrs::BGAttrs(u8 tile_index, u8 attrs)
        : index(tile_index)
        , above_sprites((attrs & 0x80) >> 7)
        , y_flip(attrs & 0x40)
        , x_flip(attrs & 0x20)
        , palette_num(attrs & 0x07)
        , bank_num((attrs & 0x08) >> 3) {}

SpriteAttrs::SpriteAttrs(u8 y, u8 x, u8 index, u8 attrs, GameMode game_mode)
        : y_pos(y)
        , x_pos(x)
        , tile_index(index)
        , behind_bg(attrs & 0x80)
        , y_flip(attrs & 0x40)
        , x_flip(attrs & 0x20) {

    if (game_mode == GameMode::DMG) {
        palette_num = (attrs & 0x10) >> 4;
        bank_num = 0;
    } else {
        palette_num = attrs & 0x07;
        bank_num = (attrs & 0x08) >> 3;
    }
};

LCD::LCD() : back_buffer(160*144) {}

void LCD::UpdateLCD() {
    // Check if the LCD has been set on or off.
    UpdatePowerOnState();

    if (!lcd_on) {
        return;
    }

    scanline_cycles += 4;

    UpdateLY();
    UpdateLYCompareSignal();

    if (current_scanline <= 143) {
        // AntonioND claims that except for scanline 0, the Mode 2 STAT interrupt happens the cycle before Mode 2
        // is entered. However, doing this causes most of Mooneye-GB's STAT timing tests to fail.
        if (scanline_cycles == ((mem->game_mode == GameMode::DMG || mem->double_speed) ? 4 : 0)) {
            SetSTATMode(2);
        } else if (scanline_cycles == ((mem->game_mode == GameMode::DMG) ? 84 : (80 << mem->double_speed))) {
            SetSTATMode(3);
            RenderScanline();
        } else if (scanline_cycles == Mode3Cycles()) {
            SetSTATMode(0);
            mem->SignalHDMA();
        }
    } else if (current_scanline == 144) {
        if (scanline_cycles == 0 && mem->console == Console::CGB) {
            stat_interrupt_signal |= Mode2CheckEnabled();
        } else if (scanline_cycles == 4 << mem->double_speed) {
            mem->RequestInterrupt(Interrupt::VBLANK);
            SetSTATMode(1);
            if (mem->console == Console::DMG) {
                // The OAM STAT interrupt is also triggered on entering Mode 1.
                stat_interrupt_signal |= Mode2CheckEnabled();
            }

            // Swap front and back buffers now that we've completed a frame.
            gameboy->SwapBuffers(back_buffer);
        }
    }

    CheckSTATInterruptSignal();
}

void LCD::UpdatePowerOnState() {
    u8 lcdc_power_on = lcdc & 0x80;
    if (lcdc_power_on ^ lcd_on) {
        lcd_on = lcdc_power_on;

        if (lcd_on) {
            // Initialize scanline cycle count (to 452/908 instead of 0, so it ticks over to 0 in UpdateLY()).
            if (mem->double_speed) {
                scanline_cycles = 908;
            } else {
                scanline_cycles = 452;
            }
            current_scanline = 153;
        } else {
            ly = 0;
            SetSTATMode(0);
            stat_interrupt_signal = 0;
            prev_interrupt_signal = 0;

            // Clear the framebuffer.
            std::fill_n(back_buffer.begin(), 160*144, 0x7FFF);
            gameboy->SwapBuffers(back_buffer);

            // An in-progress HDMA will transfer one block after the LCD switches off.
            mem->SignalHDMA();
        }
    }
}

void LCD::UpdateLY() {
    if (current_scanline == 153 && scanline_cycles == Line153Cycles()) {
        // LY is 153 only for a few machine cycle at the beginning of scanline 153, then wraps back to 0.
        ly = 0;
    }

    if (mem->game_mode == GameMode::CGB && !mem->double_speed && scanline_cycles == 452) {
        StrangeLY();
    }

    if (scanline_cycles == (456 << mem->double_speed)) {
        // Reset scanline cycle counter.
        scanline_cycles = 0;

        if (mem->game_mode == GameMode::CGB && !mem->double_speed && current_scanline != 153) {
            ly = current_scanline;
        }

        // LY does not increase at the end of scanline 153, it stays 0 until the end of scanline 0.
        // Otherwise, increment LY.
        if (current_scanline == 153) {
            if (mem->console == Console::DMG) {
                SetSTATMode(0); // Does this actually happen? Or does DMG spend the first cycle in mode 1?
            }

            current_scanline = 0;

            window_progress = 0x00;
        } else {
            current_scanline = ++ly;
        }
    }
}

int LCD::Line153Cycles() const {
    // The number of cycles where LY=153 depending on device configuration.
    if (mem->console == Console::DMG) {
        return 4;
    } else if (mem->game_mode == GameMode::DMG) {
        return 8;
    } else if (mem->double_speed) {
        return 12;
    } else {
        return 4;
    }
}

int LCD::Mode3Cycles() const {
    // The cycles taken by mode 3 increase by a number of factors.
    int cycles = 256 << mem->double_speed;

    // Mode 3 cycles increase depending on how much of the first tile is cut off by the current value of SCX.
    int scx_mod = scroll_x % 8;
    if (scx_mod > 0 && scx_mod < 5) {
        cycles += 4;
    } else if (scx_mod > 4) {
        cycles += 8;
    }

    // The number and attributes of sprites on this scanline increase the mode 3 cycles.
    // cycles += (1 + 2 * num_sprites - (num_sprites + 1) / 2) * 4;
    // The above is an expression I found through experimentation to account for the cycles added by sprites in
    // the first 10 test cases of Mooneye-GB's intr_2_mode0_timing_sprites test. It only considers the number
    // of sprites. However, I couldn't get anywhere with it.

    return cycles;
}

void LCD::StrangeLY() {
    // LY takes on strange values on the last m-cycle of a scanline in CGB single speed mode.
    if (current_scanline == 153) {
        return;
    }

    std::array<unsigned int, 9> pattern{{0, 0, 2, 0, 4, 4, 6, 0, 8}};
    if ((ly & 0x0F) == 0x0F) {
        ly = pattern[(ly >> 4) & 0x0F] << 4;
    } else {
        ly = pattern[ly & 0x07] | (ly & 0xF8);
    }
}

// Handles setting the LYC=LY compare bit and corresponding STAT interrupt on DMG. When LY changes, the LY=LYC bit is 
// set to zero that machine cycle, then on the next machine cycle, LY=LYC is set using the new LY value and the STAT
// interrupt can be fired. This will not be interrupted even if LY changes again on the second cycle (which happens 
// on scanline 153). In that case, the two events caused by an LY change begin on the following cycle.
void LCD::UpdateLYCompareSignal() {
    if (mem->console == Console::DMG) {
        if (ly_compare_equal_forced_zero) {
            SetLYCompare(ly_compare == ly_last_cycle);

            // Don't update LY_last_cycle.
            ly_compare_equal_forced_zero = false;
        } else if (ly != ly_last_cycle) {
            SetLYCompare(false);
            ly_compare_equal_forced_zero = true;
            ly_last_cycle = ly;
        } else {
            SetLYCompare(ly_compare == ly);
            ly_last_cycle = ly;
        }
    } else if (mem->double_speed) {
        if (current_scanline == 153 && scanline_cycles == 12) {
            SetLYCompare(ly_compare == ly_last_cycle);
            // Don't update LY_last_cycle.
        } else {
            SetLYCompare(ly_compare == ly_last_cycle);
            ly_last_cycle = ly;
        }
    } else {
        if (scanline_cycles == 452) {
            SetLYCompare(ly_compare == ly_last_cycle);
            // Don't update LY_last_cycle.
        } else if (ly_last_cycle == 153) {
            SetLYCompare(ly_compare == ly_last_cycle);
            ly_last_cycle = ly;
        } else {
            SetLYCompare(ly_compare == ly);
            ly_last_cycle = ly;
        }
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
    stat_interrupt_signal = false;
}

void LCD::RenderScanline() {
    std::size_t num_bg_pixels;
    if (WindowEnabled()) {
        num_bg_pixels = (window_x < 7) ? 0 : window_x - 7;
    } else {
        num_bg_pixels = 160;
    }

    if (mem->game_mode == GameMode::DMG) {
        if (BGEnabled()) {
            RenderBackground(num_bg_pixels);
        } else {
            // If disabled, we need to blank what isn't covered by the window.
            std::fill_n(row_buffer.begin(), num_bg_pixels, 0x7FFF);
        }
    } else {
        RenderBackground(num_bg_pixels);
    }

    // On CGB in DMG mode, disabling the background will also disable the window.
    if (mem->console == Console::CGB && mem->game_mode == GameMode::DMG) {
        if (BGEnabled() && WindowEnabled()) {
            RenderWindow(num_bg_pixels);
        }
    } else {
        if (WindowEnabled()) {
            RenderWindow(num_bg_pixels);
        }
    }

    if (SpritesEnabled()) {
        RenderSprites();
    }

    // Copy the row buffer into the back buffer. The last 8 pixels of the row buffer are extra off-the-end space
    // to simplify the background & window rendering code, and so they are discarded.
    std::copy(row_buffer.begin(), row_buffer.end() - 8, back_buffer.begin() + ly * 160);
}

void LCD::RenderBackground(std::size_t num_bg_pixels) {
    // The background is composed of 32x32 tiles. The scroll registers (SCY and SCX) allow the top-left corner of
    // the screen to be positioned anywhere on the background, and the background wraps around when it hits the edge.

    // Determine which row we need to fetch from the current values of SCY and LY.
    unsigned int row_num = ((scroll_y + ly) / 8) % tile_map_row_len;
    InitTileMap(BGTileMapStartAddr() + row_num * tile_map_row_len);

    FetchTiles();

    // Determine which row of pixels we're on, and in which tile we start reading data.
    std::size_t tile_row = ((scroll_y + ly) % 8) * 2;
    std::size_t start_tile = scroll_x / 8;
    auto tile_iter = tile_data.begin() + start_tile;

    // If necessary, throw away the first few pixels of the first tile, based on SCX.
    std::size_t row_pixel = RenderFirstTile(0, start_tile, tile_row, scroll_x % 8);

    // Increment the tile index to the next tile, and wrap around if we hit the end.
    if (++tile_iter == tile_data.end()) {
        tile_iter = tile_data.begin();
    }

    while (row_pixel < num_bg_pixels) {
        // If this tile has the Y flip flag set, decode the mirrored row in the other half of the tile.
        DecodePaletteIndices(tile_iter->tile, (tile_iter->y_flip) ? (14 - tile_row) : tile_row);

        // If this tile has the X flip flag set, reverse the pixels.
        if (tile_iter->x_flip) {
            std::reverse(pixel_colours.begin(), pixel_colours.end());
        }

        // Record the palette index for each pixel and the bg priority bit.
        for (const auto& pixel_colour : pixel_colours) {
            row_bg_info[row_pixel++] = (pixel_colour << 1) | tile_iter->above_sprites;
        }
        row_pixel -= 8;

        if (mem->game_mode == GameMode::DMG) {
            GetPixelColoursFromPaletteDMG(bg_palette_dmg, false);
        } else {
            GetPixelColoursFromPaletteCGB(tile_iter->palette_num, false);
        }

        // Copy the pixels to the row buffer.
        for (const auto& pixel_colour : pixel_colours) {
            row_buffer[row_pixel++] = pixel_colour;
        }

        if (++tile_iter == tile_data.end()) {
            tile_iter = tile_data.begin();
        }
    }
}

void LCD::RenderWindow(std::size_t num_bg_pixels) {
    // The window is composed of 32x32 tiles (of which only 21x18 tiles can be seen). Unlike the background, the
    // window cannot be scrolled; it is always displayed from its top-left corner and does not wrap around.
    // Instead, the position of its top-left corner can be set with the WY and WX registers.

    // Determine which row we need to fetch from the current internal value of the window progression.
    InitTileMap(WindowTileMapStartAddr() + (window_progress / 8) * tile_map_row_len);

    FetchTiles();

    // While the window is enabled, each row of the window is drawn successively starting from the top. If it is
    // disabled while a frame is being drawn and later re-enabled during the same frame, the window will resume
    // drawing from the row at which it left off; hence, the window progress must be tracked separately of LY.

    // Determine which row of pixels we're on.
    std::size_t tile_row = ((window_progress) % 8) * 2;
    auto tile_iter = tile_data.begin();

    // If necessary, throw away the first few pixels of the first tile, based on WX.
    std::size_t row_pixel = RenderFirstTile(num_bg_pixels, 0, tile_row, (window_x < 7) ? 7 - window_x : 0);
    ++tile_iter;

    while (row_pixel < 160) {
        // If this tile has the Y flip flag set, decode the mirrored row in the other half of the tile.
        DecodePaletteIndices(tile_iter->tile, (tile_iter->y_flip) ? (14 - tile_row) : tile_row);

        // If this tile has the X flip flag set, reverse the pixels.
        if (tile_iter->x_flip) {
            std::reverse(pixel_colours.begin(), pixel_colours.end());
        }

        // Record the palette index for each pixel and the bg priority bit.
        for (const auto& pixel_colour : pixel_colours) {
            row_bg_info[row_pixel++] = (pixel_colour << 1) | tile_iter->above_sprites;
        }
        row_pixel -= 8;

        if (mem->game_mode == GameMode::DMG) {
            GetPixelColoursFromPaletteDMG(bg_palette_dmg, false);
        } else {
            GetPixelColoursFromPaletteCGB(tile_iter->palette_num, false);
        }

        // Copy the pixels to the row buffer.
        for (const auto& pixel_colour : pixel_colours) {
            row_buffer[row_pixel++] = pixel_colour;
        }

        ++tile_iter;
    }

    // Increment internal window progression.
    ++window_progress;
}

std::size_t LCD::RenderFirstTile(std::size_t start_pixel, std::size_t start_tile, std::size_t tile_row,
                                 std::size_t throwaway) {
    auto& bg_tile = tile_data[start_tile];

    // If this tile has the Y flip flag set, decode the mirrored row in the other half of the tile.
    DecodePaletteIndices(bg_tile.tile, (bg_tile.y_flip) ? (14 - tile_row) : tile_row);

    // If this tile has the X flip flag set, reverse the pixels.
    if (bg_tile.x_flip) {
        std::reverse(pixel_colours.begin(), pixel_colours.end());
    }

    // Record the palette index for each pixel and the bg priority bit.
    for (std::size_t pixel = throwaway; pixel < 8; ++pixel) {
        row_bg_info[start_pixel++] = (pixel_colours[pixel] << 1) | bg_tile.above_sprites;
    }
    start_pixel -= 8 - throwaway;

    if (mem->game_mode == GameMode::DMG) {
        GetPixelColoursFromPaletteDMG(bg_palette_dmg, false);
    } else {
        GetPixelColoursFromPaletteCGB(bg_tile.palette_num, false);
    }

    // Throw away the first pixels of the tile.
    for (std::size_t pixel = throwaway; pixel < 8; ++pixel) {
        row_buffer[start_pixel++] = pixel_colours[pixel];
    }

    // Return the number of pixels written to the row buffer.
    return start_pixel;
}

void LCD::RenderSprites() {
    SearchOAM();

    FetchSpriteTiles();

    // Each row of 8 pixels in a tile is 2 bytes. The first byte contains the low bit of the palette index for
    // each pixel, and the second byte contains the high bit of the palette index.
    for (const auto& sa : oam_sprites) {
        // Determine which row of the sprite tile is being drawn.
        std::size_t tile_row = (ly - (sa.y_pos - 16));

        // If this sprite has the Y flip flag set, get the mirrored row in the other half of the sprite.
        if (sa.y_flip) {
            tile_row = (SpriteSize() - 1) - tile_row;
        }

        // Two bytes per tile row.
        tile_row *= 2;

        DecodePaletteIndices(sa.sprite_tiles, tile_row);

        if (mem->game_mode == GameMode::DMG) {
            GetPixelColoursFromPaletteDMG((sa.palette_num) ? obj_palette_dmg1 : obj_palette_dmg0, true);
        } else {
            GetPixelColoursFromPaletteCGB(sa.palette_num, true);
        }

        // If this sprite has the X flip flag set, reverse the pixels.
        if (sa.x_flip) {
            std::reverse(pixel_colours.begin(), pixel_colours.end());
        }

        auto pixel_iter = pixel_colours.cbegin(), pixel_end_iter = pixel_colours.cend();

        // If the sprite's X position is less than 8 or greater than 159, part of the sprite will be cut off.
        std::size_t row_pixel = sa.x_pos - 8;
        if (sa.x_pos < 8) {
            pixel_iter += 8 - sa.x_pos;
            row_pixel = 0;
        } else if (sa.x_pos > 160) {
            pixel_end_iter -= (sa.x_pos - 160);
        }

        // If the sprite is drawn below the background, then it is only drawn on pixels of colour 0 for the palette
        // of that tile.
        u16 bg_colour_mask = 0x0000, bg_priority_mask = 0x0000;
        if (mem->game_mode == GameMode::CGB) {
            // If the BG is "disabled" on CGB, both BG and OAM priority flags are ignored and the sprite is drawn
            // above the background.
            if (BGEnabled()) {
                if (sa.behind_bg) {
                    bg_colour_mask = 0x0006;
                } else {
                    // Draw the sprite above the background, unless the BG priority bit is set.
                    bg_priority_mask = 0x0001;
                }
            }
        } else if (sa.behind_bg) {
            bg_colour_mask = 0x0006;
        }

        while (pixel_iter != pixel_end_iter) {
            bool pixel_transparent = *pixel_iter & 0x8000;
            u16 per_pixel_mask = bg_colour_mask | ((row_bg_info[row_pixel] & bg_priority_mask) ? 0x0006 : 0x0000);

            if (!pixel_transparent && (row_bg_info[row_pixel] & per_pixel_mask) == 0) {
                row_buffer[row_pixel] = *pixel_iter;
            }
            ++pixel_iter;
            ++row_pixel;
        }
    }
}

void LCD::SearchOAM() {
    // The sprite_gap is the distance between the bottom of the sprite and its Y position (8 for 8x8, 0 for 8x16).
    unsigned int sprite_gap = SpriteSize() % 16;
    // The tile index mask is 0xFF for 8x8, 0xFE for 8x16.
    u8 index_mask = (sprite_gap >> 3) | 0xFE;

    // Store the first 10 sprites from OAM which are on this scanline.
    oam_sprites.clear();
    for (std::size_t i = 0; i < oam.size(); i += 4) {
        // Check that the sprite is not off the screen.
        if (oam[i] > sprite_gap && oam[i] < 160) {
            // Check that the sprite is on the current scanline.
            if (ly < oam[i] - sprite_gap && static_cast<int>(ly) >= static_cast<int>(oam[i]) - 16) {
                oam_sprites.emplace_front(oam[i], oam[i+1], oam[i+2] & index_mask, oam[i+3], mem->game_mode);
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

    if (mem->game_mode == GameMode::DMG) {
        // Sprite are drawn in descending X order. If two sprites overlap, the one that has a lower position in OAM
        // is drawn on top. oam_sprites already contains the sprites for this line in decreasing OAM position, so
        // we sort them by decreasing X position. In CGB mode, sprites are always drawn according to OAM position.
        std::stable_sort(oam_sprites.begin(), oam_sprites.end(), [](SpriteAttrs sa1, SpriteAttrs sa2) {
            return sa2.x_pos < sa1.x_pos;
        });
    }
}

void LCD::InitTileMap(u16 tile_map_addr) {
    // The tile maps are located at 0x9800-0x9BFF and 0x9C00-0x9FFF. They consist of 32 rows of 32 bytes each
    // which index the tileset.

    // Get the current row of tile indices from VRAM.
    std::array<u8, tile_map_row_len> row_tile_map;
    mem->CopyFromVRAM(tile_map_addr, tile_map_row_len, 0, row_tile_map.begin());

    tile_data.clear();

    if (mem->game_mode == GameMode::DMG) {
        for (std::size_t i = 0; i < row_tile_map.size(); ++i) {
            tile_data.emplace_back(row_tile_map[i]);
        }
    } else {
        // Get the current row of background tile attributes from VRAM.
        std::array<u8, tile_map_row_len> row_attr_map;
        mem->CopyFromVRAM(tile_map_addr, tile_map_row_len, 1, row_attr_map.begin());

        for (std::size_t i = 0; i < row_tile_map.size(); ++i) {
            tile_data.emplace_back(row_tile_map[i], row_attr_map[i]);
        }
    }
}

void LCD::FetchTiles() {
    // The background tiles are located at either 0x8000-0x8FFF or 0x8800-0x97FF. For the first region, the
    // tile map indices are unsigned offsets from 0x8000; for the second region, the indices are signed
    // offsets from 0x9000.

    u16 region_start_addr = TileDataStartAddr();
    if (region_start_addr == 0x9000) {
        // Signed tile data region.
        for (auto& bg_tile : tile_data) {
            u16 tile_addr = region_start_addr + static_cast<s8>(bg_tile.index) * static_cast<s8>(tile_bytes);
            mem->CopyFromVRAM(tile_addr, tile_bytes, bg_tile.bank_num, bg_tile.tile.begin());
        }
    } else {
        // Unsigned tile data region.
        for (auto& bg_tile : tile_data) {
            u16 tile_addr = region_start_addr + bg_tile.index * tile_bytes;
            mem->CopyFromVRAM(tile_addr, tile_bytes, bg_tile.bank_num, bg_tile.tile.begin());
        }
    }
}

void LCD::FetchSpriteTiles() {
    std::size_t tile_size = tile_bytes;
    if (SpriteSize() == 16) {
        tile_size *= 2;
    }

    // Sprite tiles can only be located in 0x8000-0x8FFF.
    for (auto& sa : oam_sprites) {
        u16 tile_addr = 0x8000 | (static_cast<u16>(sa.tile_index) << 4);
        mem->CopyFromVRAM(tile_addr, tile_size, sa.bank_num, sa.sprite_tiles.begin());
    }
}

void LCD::GetPixelColoursFromPaletteDMG(u8 palette, bool sprite) {
    for (auto& colour : pixel_colours) {
        if (sprite && colour == 0) {
            // Palette index 0 is transparent for sprites. Set the alpha bit.
            colour |= 0x8000;
        } else {
            colour = shades[(palette >> (colour * 2)) & 0x03];
        }
    }
}

void LCD::GetPixelColoursFromPaletteCGB(int palette_num, bool sprite) {
    if (sprite) {
        for (auto& colour : pixel_colours) {
            if (colour == 0) {
                // Palette index 0 is transparent for sprites. Set the alpha bit.
                colour |= 0x8000;
            } else {
                std::size_t index = palette_num * 8 + colour * 2;
                colour = (static_cast<u16>((obj_palette_data[index + 1] & 0x7F)) << 8) | obj_palette_data[index];
            }
        }
    } else {
        for (auto& colour : pixel_colours) {
            std::size_t index = palette_num * 8 + colour * 2;
            colour = (static_cast<u16>((bg_palette_data[index + 1] & 0x7F)) << 8) | bg_palette_data[index];
        }
    }
}

} // End namespace Core
