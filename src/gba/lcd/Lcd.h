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

#include <vector>

#include "common/CommonTypes.h"
#include "gba/memory/IOReg.h"

namespace Gba {

class Core;
class Bg;

class Lcd {
public:
    Lcd(const std::vector<u16>& _pram, const std::vector<u16>& _vram, const std::vector<u32>& _oam, Core& _core);
    ~Lcd();

    IOReg control       = {0x0000, 0xFFF7, 0xFFF7};
    IOReg green_swap    = {0x0000, 0x0001, 0x0001};
    IOReg status        = {0x0000, 0xFF3F, 0xFF38};
    IOReg vcount        = {0x0000, 0x00FF, 0x0000};

    IOReg win0_width    = {0x0000, 0x0000, 0xFFFF};
    IOReg win1_width    = {0x0000, 0x0000, 0xFFFF};
    IOReg win0_height   = {0x0000, 0x0000, 0xFFFF};
    IOReg win1_height   = {0x0000, 0x0000, 0xFFFF};
    IOReg winin         = {0x0000, 0x3F3F, 0x3F3F};
    IOReg winout        = {0x0000, 0x3F3F, 0x3F3F};

    IOReg mosaic        = {0x0000, 0x0000, 0xFFFF};
    IOReg blend_control = {0x0000, 0x3FFF, 0x3FFF};
    IOReg blend_alpha   = {0x0000, 0x1F1F, 0x1F1F};
    IOReg blend_fade    = {0x0000, 0x0000, 0x001F};

    std::vector<Bg> bgs;

    const std::vector<u16>& pram;
    const std::vector<u16>& vram;
    const std::vector<u32>& oam;

    static constexpr int h_pixels = 240;
    static constexpr int v_pixels = 160;
    static constexpr u16 alpha_bit = 0x8000;

    void Update(int cycles);

private:
    Core& core;

    std::vector<u16> back_buffer;

    int scanline_cycles = 0;

    void DrawScanline();

    // Control flags
    int BgMode() const { return control & 0x7; }
    int DisplayFrame() const { return (control >> 4) & 0x1; }
    bool HBlankFree() const { return control & 0x20; }
    bool ObjMapping1D() const { return control & 0x40; }
    bool ForcedBlank() const { return control & 0x80; }
    bool BgEnabled(int bg_id) const { return control & (0x100 << bg_id); }
    bool ObjEnabled() const { return control & 0x1000; }
    bool WinEnabled(int win_id) const { return control & (0x2000 << win_id); }
    bool ObjWinEnabled() const { return control & 0x8000; }

    // Status flags
    static constexpr u16 vblank_flag = 0x01;
    static constexpr u16 hblank_flag = 0x02;
    static constexpr u16 vcount_flag = 0x04;
    static constexpr u16 vblank_irq  = 0x08;
    static constexpr u16 hblank_irq  = 0x10;
    static constexpr u16 vcount_irq  = 0x20;

    int VTrigger() const { return status >> 8; }
};

} // End namespace Gba
