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

#include <array>
#include <deque>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

namespace Core {

class Memory;
class GameBoy;

struct SpriteAttrs {
    SpriteAttrs(u8 y, u8 x, u8 index, u8 attributes) : y_pos(y), x_pos(x), tile_index(index), attrs(attributes) {};

    u8 y_pos, x_pos, tile_index, attrs;
    std::array<u8, 2*16> sprite_tiles;
};

class LCD {
public:
    void UpdateLCD();

    void LinkToMemory(Memory* memory) { mem = memory; }
    void LinkToGameBoy(GameBoy* gb) { gameboy = gb; }

    // Debug functions
    void PrintRegisterState();

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
    u8 lcdc = 0x91; // TODO: Verify that 0x91 is the correct initial value for this register.
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
    //     bits 7-6: background colour 3
    //     bits 5-4: background colour 2
    //     bits 3-2: background colour 1
    //     bits 1-0: background colour 0
    u8 bg_palette = 0xFC;
    // OBP0 register: 0xFF48
    u8 obj_palette0 = 0xFF;
    // OBP1 register: 0xFF49
    u8 obj_palette1 = 0xFF;
    // WY register: 0xFF4A
    u8 window_y = 0x00;
    // WX register: 0xFF4B
    u8 window_x = 0x00;
private:
    Memory* mem;
    GameBoy* gameboy;

    u8 lcd_on = 0x80;
    void UpdatePowerOnState();

    int scanline_cycles = 452; // This should be set in constructor to adapt for CGB double speed.
    void UpdateLY();

    bool stat_interrupt_signal = false, prev_interrupt_signal = false;
    void CheckSTATInterruptSignal();

    // LY=LYC interrupt
    u8 LY_last_cycle = 0xFF;
    bool LY_compare_equal_forced_zero = false;
    void UpdateLYCompareSignal();

    // Drawing
    static constexpr std::size_t num_tiles = 32;
    static constexpr std::size_t tile_map_row_bytes = 32;
    static constexpr std::size_t tile_bytes = 16;
    const std::array<unsigned int, 4> shades{0xFFFFFF00, 0xAAAAAA00, 0x55555500, 0x00000000};

    std::array<u8, num_tiles> row_tile_map;
    std::array<s8, num_tiles> signed_row_tile_map;
    std::array<u8, num_tiles*tile_bytes> tile_data;

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
    std::array<u8, 40*4> oam_ram;

    std::array<u32, 8> pixel_colours;
    std::array<u32, 176> bg_row_pixels;
    std::array<u32, 168> win_row_pixels;
    std::array<u32, 160> row_buffer{};
    std::array<u32, 160*144> framebuffer{};

    u8 window_y_frame_val = 0x00;
    u8 window_progress = 0x00;

    void RenderScanline();
    void RenderBackground();
    void RenderWindow();
    void RenderSprites();
    template<typename T, std::size_t N>
    void FetchTiles(const std::array<T, N>& tile_indicies);
    void FetchSpriteTiles(std::deque<SpriteAttrs>& sprites);
    void DecodePixelColoursFromPalette(u8 lsb, u8 msb, u8 palette, bool sprite);

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
