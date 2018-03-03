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

#pragma once

#include <array>
#include <vector>

#include "common/CommonTypes.h"
#include "gba/memory/IOReg.h"

namespace Gba {

class Lcd;

using Tile = std::array<u8, 64>;

struct BgTile {
    BgTile(u16 map_entry)
            : num(map_entry & 0x3FF)
            , h_flip(map_entry & 0x400)
            , v_flip(map_entry & 0x800)
            , palette(map_entry >> 12) {}

    int num;
    bool h_flip;
    bool v_flip;
    int palette;

    Tile data;
};

class Bg {
public:
    Bg(Lcd& _lcd)
            : lcd(_lcd) {}

    IOReg control    = {0x0000, 0xFFCF, 0xFFCF};
    IOReg scroll_x   = {0x0000, 0x0000, 0x01FF};
    IOReg scroll_y   = {0x0000, 0x0000, 0x01FF};

    IOReg affine_a   = {0x0000, 0x0000, 0xFFFF};
    IOReg affine_b   = {0x0000, 0x0000, 0xFFFF};
    IOReg affine_c   = {0x0000, 0x0000, 0xFFFF};
    IOReg affine_d   = {0x0000, 0x0000, 0xFFFF};

    IOReg offset_x_l = {0x0000, 0x0000, 0xFFFF};
    IOReg offset_x_h = {0x0000, 0x0000, 0x0FFF};
    IOReg offset_y_l = {0x0000, 0x0000, 0xFFFF};
    IOReg offset_y_h = {0x0000, 0x0000, 0x0FFF};

    std::array<u16, 240> scanline;

    void GetRowMapInfo();
    void GetTileData();
    void DrawScanline();

    // Control flags
    static constexpr int kbyte = 1024;

    int Priority() const { return control & 0x3; }
    int TileBase() const { return ((control >> 2) & 0x3) * 16 * kbyte; }
    bool Mosaic() const { return control & 0x40; }
    bool SinglePalette() const { return control & 0x80; }
    int MapBase() const { return ((control >> 8) & 0x1F) * 2 * kbyte; }
    bool Wraparound() const { return control & 0x2000; }

    enum class Regular {Size32x32 = 0,
                        Size64x32 = 1,
                        Size32x64 = 2,
                        Size64x64 = 3};

    enum class Affine {Size16x16   = 0,
                       Size32x32   = 1,
                       Size64x64   = 2,
                       Size128x128 = 3};

    template<typename T>
    T ScreenSize() const { return static_cast<T>(control >> 14); }

private:
    Lcd& lcd;

    std::vector<BgTile> tiles;
};

} // End namespace Gba
