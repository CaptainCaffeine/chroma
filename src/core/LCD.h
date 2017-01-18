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

#pragma once

#include <vector>
#include <array>
#include <deque>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

// Forward declaration for cross-namespace friend declaration.
namespace Log { class Logging; }

namespace Core {

class Memory;
class GameBoy;

struct BGAttrs {
    BGAttrs(u8 tile_index);
    BGAttrs(u8 tile_index, u8 attrs);

    const u8 index = 0, above_sprites = 0;
    const bool y_flip = false, x_flip = false;
    const int palette_num = 0, bank_num = 0;

    std::array<u8, 16> tile;
};

struct SpriteAttrs {
    SpriteAttrs(u8 y, u8 x, u8 index, u8 attrs, GameMode game_mode);

    u8 y_pos, x_pos, tile_index;
    bool behind_bg, y_flip, x_flip;
    int palette_num, bank_num;

    std::array<u8, 32> sprite_tiles;
};

class LCD {
    friend class Log::Logging;
public:
    LCD();

    void UpdateLCD();

    void LinkToMemory(Memory* memory) { mem = memory; }
    void LinkToGameBoy(GameBoy* gb) { gameboy = gb; }

    void SetSTATSignal() { stat_interrupt_signal = true; }

    // ******** LCD I/O registers ********
    // LCDC register: 0xFF40
    //     bit 7: LCD On
    //     bit 6: Window Tilemap Region (0=0x9800-0x9BFF, 1=0x9C00-0x9FFF)
    //     bit 5: Window Enable
    //     bit 4: BG and Window Tile Data Region (0=0x8800-0x97FF, 1=0x8000-0x8FFF)
    //     bit 3: BG Tilemap Region (0=0x9800-0x9BFF, 1=0x9C00-0x9FFF)
    //     bit 2: Sprite Size (0=8x8, 1=8x16)
    //     bit 1: Sprites Enabled
    //     bit 0: BG Enabled (0=On DMG, this sets the background to white.
    //                          On CGB in DMG mode, this disables both the window and background. 
    //                          In CGB mode, this gives all sprites priority over the background and window.)
    u8 lcdc = 0x91;
    // STAT register: 0xFF41
    //     bit 6: LY=LYC Check Enable
    //     bit 5: Mode 2 OAM Check Enable
    //     bit 4: Mode 1 VBLANK Check Enable
    //     bit 3: Mode 0 HBLANK Check Enable
    //     bit 2: LY=LYC Compare Signal (1 implies LY=LYC)
    //     bits 1&0: Screen Mode (0=HBLANK, 1=VBLANK, 2=Searching OAM, 3=Transferring Data to LCD driver)
    u8 stat = 0x01;
    // SCY register: 0xFF42
    u8 scroll_y = 0x00;
    // SCX register: 0xFF43
    u8 scroll_x = 0x00;
    // LY register: 0xFF44
    u8 ly = 0x00;
    // LYC register: 0xFF45
    u8 ly_compare = 0x00;

    // BGP register: 0xFF47
    //     bits 7-6: Background Colour 3
    //     bits 5-4: Background Colour 2
    //     bits 3-2: Background Colour 1
    //     bits 1-0: Background Colour 0
    u8 bg_palette_dmg = 0xFC;
    // OBP0 register: 0xFF48
    u8 obj_palette_dmg0 = 0xFF;
    // OBP1 register: 0xFF49
    u8 obj_palette_dmg1 = 0xFF;

    // WY register: 0xFF4A
    u8 window_y = 0x00;
    // WX register: 0xFF4B
    u8 window_x = 0x00;

    // BGPI register: 0xFF68
    //     bits 7: Auto Increment (1=Increment after write)
    //     bits 5-0: Palette Index (0x00-0x3F)
    u8 bg_palette_index = 0x00;
    // BGPD register: 0xFF69
    //     bits 7-0 in first byte, 8-14 in second byte
    //     bits 14-10: Blue Intensity
    //     bits 9-5:   Green Intensity
    //     bits 0-4:   Red Intensity
    std::array<u8, 64> bg_palette_data{};
    // OBPI register: 0xFF6A
    //     bits 7: Auto Increment (1=Increment after write)
    //     bits 5-0: Palette Index (0x00-0x3F)
    u8 obj_palette_index = 0x00;
    // OBP1 register: 0xFF6B
    //     bits 7-0 in first byte, 8-14 in second byte
    //     bits 14-10: Blue Intensity
    //     bits 9-5:   Green Intensity
    //     bits 0-4:   Red Intensity
    std::array<u8, 64> obj_palette_data{};

private:
    Memory* mem;
    GameBoy* gameboy;

    u8 lcd_on = 0x80;
    void UpdatePowerOnState();

    int scanline_cycles = 452;
    u8 current_scanline = 0;
    void UpdateLY();
    int Line153Cycles() const;
    int Mode3Cycles() const;
    void StrangeLY();

    bool stat_interrupt_signal = false, prev_interrupt_signal = false;
    void CheckSTATInterruptSignal();

    // LY=LYC interrupt
    u8 ly_last_cycle = 0xFF;
    bool ly_compare_equal_forced_zero = false;
    void UpdateLYCompareSignal();

    // Drawing
    static constexpr std::size_t num_tiles = 32;
    static constexpr std::size_t tile_map_row_bytes = 32;
    static constexpr std::size_t tile_bytes = 16;
    const std::array<u16, 4> shades{{0x7FFF, 0x56B5, 0x294A, 0x0000}};

    std::array<u8, num_tiles> row_tile_map;
    std::array<u8, num_tiles> row_attr_map;
    std::vector<BGAttrs> tile_data;

    // The Sprite Attribute Table (OAM) contains 40 sprite attributes each 4 bytes long.
    // Byte 0: the Y position of the sprite, minus 16.
    // Byte 1: the X position of the sprite, minus 8.
    // Byte 2: the tile number, an unsigned offset which indicates a tile at 0x8NN0. Sprite tiles have the same
    //         format as background tiles. If the current sprite size is 8x16, bit 0 of this value is ignored, as
    //         the sprites take up 2 tiles of space.
    // Byte 3: the sprite attributes:
    //     Bit 7: Sprite-background priority (0=sprite above BG, 1=sprite behind BG colours 1-3. Sprites are
    //            always on top of colour 0.)
    //     Bit 6: Y flip
    //     Bit 5: X flip
    //     Bit 4: Palette number (0=OBP0, 1=OBP1) (DMG mode only)
    //     Bit 3: Tile VRAM bank (0=bank 0, 1=bank 1) (CGB mode only)
    //     Bit 2-0: Palette number (selects OBP0-7) (CGB mode only)
    std::array<u8, 160> oam_ram;
    std::deque<SpriteAttrs> oam_sprites;

    std::array<u16, 8> pixel_colours;
    std::array<u16, 168> row_buffer;
    std::array<u16, 168> row_bg_info;
    std::vector<u16> back_buffer;

    u8 window_y_frame_val = 0x00;
    u8 window_progress = 0x00;

    void RenderScanline();
    void RenderBackground(std::size_t num_bg_pixels);
    void RenderWindow(std::size_t num_bg_pixels);
    std::size_t RenderFirstTile(std::size_t start_pixel, std::size_t start_tile, std::size_t tile_row,
                                std::size_t throwaway);
    void RenderSprites();
    void SearchOAM();
    void FetchTiles();
    void FetchSpriteTiles();
    template<std::size_t N>
    void DecodePaletteIndexes(const std::array<u8, N>& tile, const std::size_t tile_row);
    void GetPixelColoursFromPaletteDMG(u8 palette, bool sprite);
    void GetPixelColoursFromPaletteCGB(int palette_num, bool sprite);

    // STAT functions
    void SetSTATMode(unsigned int mode) { stat = (stat & 0xFC) | mode; }
    unsigned int STATMode() const { return stat & 0x03; }
    void SetLYCompare(bool eq) { if (eq) { stat |= 0x04; } else { stat &= ~0x04; } }
    bool LYCompareEqual() const { return stat & 0x04; }
    bool LYCompareCheckEnabled() const { return stat & 0x40; }
    bool Mode2CheckEnabled() const { return stat & 0x20; }
    bool Mode1CheckEnabled() const { return stat & 0x10; }
    bool Mode0CheckEnabled() const { return stat & 0x08; }

    // LCDC functions
    u16 TileDataStartAddr() const { return (lcdc & 0x10) ? 0x8000 : 0x9000; }
    bool BGEnabled() const { return lcdc & 0x01; }
    u16 BGTileMapStartAddr() const { return (lcdc & 0x08) ? 0x9C00 : 0x9800; }
    // The window can be disabled by either disabling it in LCDC or by pushing it off the screen.
    bool WindowEnabled() const { return (lcdc & 0x20)
                                        && (window_x < 167)
                                        && (window_y_frame_val < 144)
                                        && (ly >= window_y_frame_val); }
    u16 WindowTileMapStartAddr() const { return (lcdc & 0x40) ? 0x9C00 : 0x9800; }
    bool SpritesEnabled() const { return lcdc & 0x02; }
    int SpriteSize() const { return (lcdc & 0x04) ? 16 : 8; }
};

} // End namespace Core
