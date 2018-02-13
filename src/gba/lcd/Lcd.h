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

#include "common/CommonTypes.h"
#include "gba/memory/IOReg.h"

namespace Gba {

class Core;

class Bg {
public:
    IOReg control = {0x0000, 0xFFCF, 0xFFCF};
    IOReg scroll_x = {0x0000, 0x0000, 0x01FF};
    IOReg scroll_y = {0x0000, 0x0000, 0x01FF};

    IOReg affine_a = {0x0000, 0x0000, 0xFFFF};
    IOReg affine_b = {0x0000, 0x0000, 0xFFFF};
    IOReg affine_c = {0x0000, 0x0000, 0xFFFF};
    IOReg affine_d = {0x0000, 0x0000, 0xFFFF};

    IOReg offset_x_l = {0x0000, 0x0000, 0xFFFF};
    IOReg offset_x_h = {0x0000, 0x0000, 0x0FFF};
    IOReg offset_y_l = {0x0000, 0x0000, 0xFFFF};
    IOReg offset_y_h = {0x0000, 0x0000, 0x0FFF};
};

class Lcd {
public:
    Lcd(Core& _core);

    IOReg control = {0x0000, 0xFFF7, 0xFFF7};
    IOReg green_swap = {0x0000, 0x0001, 0x0001};
    IOReg status = {0x0000, 0xFF3F, 0xFF38};
    IOReg vcount = {0x0000, 0x00FF, 0x0000};

    std::array<Bg, 4> bg{};

    IOReg win0_width = {0x0000, 0x0000, 0xFFFF};
    IOReg win1_width = {0x0000, 0x0000, 0xFFFF};
    IOReg win0_height = {0x0000, 0x0000, 0xFFFF};
    IOReg win1_height = {0x0000, 0x0000, 0xFFFF};
    IOReg winin = {0x0000, 0x3F3F, 0x3F3F};
    IOReg winout = {0x0000, 0x3F3F, 0x3F3F};

    IOReg mosaic = {0x0000, 0x0000, 0xFFFF};
    IOReg blend_control = {0x0000, 0x3FFF, 0x3FFF};
    IOReg blend_alpha = {0x0000, 0x1F1F, 0x1F1F};
    IOReg blend_fade = {0x0000, 0x0000, 0x001F};

    void Update(int cycles);
private:
    Core& core;

    int scanline_cycles = 0;

    // DISPSTAT flags
    static constexpr u16 vblank_flag = 0x01;
    static constexpr u16 hblank_flag = 0x02;
    static constexpr u16 vcount_flag = 0x04;
    static constexpr u16 vblank_irq  = 0x08;
    static constexpr u16 hblank_irq  = 0x10;
    static constexpr u16 vcount_irq  = 0x20;

    int VTrigger() const { return status >> 8; }
};

} // End namespace Gba
