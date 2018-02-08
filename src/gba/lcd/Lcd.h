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

#include "common/CommonTypes.h"
#include "gba/memory/IOReg.h"

namespace Gba {

class Core;

class Lcd {
public:
    Lcd(Core& _core);

    IOReg dispcnt = {0x0000, 0xFFF7, 0xFFF7};
    IOReg dispstat = {0x0000, 0xFF3F, 0xFF38};
    IOReg vcount = {0x0000, 0x00FF, 0x0000};

    void Update(int cycles);
private:
    Core& core;

    int scanline_cycles = 0;

    // Dispstat flags
    static constexpr u16 vblank_flag = 0x01;
    static constexpr u16 hblank_flag = 0x02;
    static constexpr u16 vcount_flag = 0x04;
    static constexpr u16 vblank_irq  = 0x08;
    static constexpr u16 hblank_irq  = 0x10;
    static constexpr u16 vcount_irq  = 0x20;

    int VTrigger() const { return dispstat >> 8; }
};

} // End namespace Gba
