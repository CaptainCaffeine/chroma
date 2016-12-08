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

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"
#include "core/Memory.h"

namespace Core {

class LCD {
public:
    LCD(Memory& memory);

    void UpdateLCD();

    const u32* GetRawPointerToFramebuffer() const { return framebuffer.data(); };

    void PrintRegisterState();
private:
    Memory& mem;

    u8 lcd_on = 0x80;
    void UpdatePowerOnState();

    int scanline_cycles = 452; // This should be set in constructor to adapt for CGB double speed.
    void UpdateLY();

    bool stat_interrupt_signal = false, prev_interrupt_signal = false;

    // LY=LYC interrupt
    u8 LY_last_cycle = 0xFF;
    bool LY_compare_equal_forced_zero = false;
    void UpdateLYCompareSignal();

    // Drawing
    static constexpr std::size_t num_tiles = 32;
    static constexpr std::size_t tile_bytes = 16;
    const std::array<unsigned int, 4> shades{0xFFFFFF00, 0xAAAAAA00, 0x55555500, 0x00000000};
    std::array<u8, num_tiles> row_tile_map;
    std::array<s8, num_tiles> signed_row_tile_map;
    std::array<u8, num_tiles*tile_bytes> tile_data;

    std::array<u32, 176> row_pixels;
    std::array<u32, 160*144> framebuffer{};

    void RenderScanline();

    template<typename T, std::size_t N>
    void FetchTiles(const std::array<T, N>& tile_indicies) {
        u16 region_start_addr = TileDataStartAddr();
        auto tile_data_iter = tile_data.begin();
        // T is either u8 or s8, depending on the current tile data region.
        for (T index : tile_indicies) {
            u16 tile_addr = region_start_addr + index * tile_bytes;
            mem.CopyFromVRAM(tile_addr, tile_bytes, tile_data_iter);
            tile_data_iter += tile_bytes;
        }
    }

    // STAT functions
    void SetSTATMode(unsigned int mode) { mem.stat = (mem.stat & 0xFC) | mode; }
    unsigned int STATMode() const { return mem.stat & 0x03; }
    void SetLYCompare(bool eq) { if (eq) { mem.stat |= 0x04; } else { mem.stat &= ~0x04; } }
    bool LYCompareEqual() const { return mem.stat & 0x04; }
    bool LYCompareCheckEnabled() const { return mem.stat & 0x40; }
    bool Mode2CheckEnabled() const { return mem.stat & 0x20; }
    bool Mode1CheckEnabled() const { return mem.stat & 0x10; }
    bool Mode0CheckEnabled() const { return mem.stat & 0x08; }

    // LCDC functions
    // Separate BG and window functions? Or have a BG and Window enum passed as parameter? Perhaps
    // enum class Drawing{BG, Window};
    // GetTileMapStartAddr(Drawing drawing) {u8 mask = (drawing == Drawing::BG) ? 0x08 : 0x40; ... test & return addr}
    u16 TileDataStartAddr() const { return (mem.lcdc & 0x10) ? 0x8000 : 0x9000; }
    bool BGEnabled() const { return mem.lcdc & 0x01; }
    u16 BGTileMapStartAddr() const { return (mem.lcdc & 0x08) ? 0x9C00 : 0x9800; }
    bool WindowEnabled() const { return mem.lcdc & 0x20; }
    u16 WindowTileMapStartAddr() const { return (mem.lcdc & 0x40) ? 0x9C00 : 0x9800; }
    bool SpritesEnabled() const { return mem.lcdc & 0x02; }
};

} // End namespace Core
