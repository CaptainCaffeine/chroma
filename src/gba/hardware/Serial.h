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

class Serial {
public:
    explicit Serial(Core&) {}

    IOReg data0          = {0x0000, 0xFFFF, 0xFFFF};
    IOReg data1          = {0x0000, 0xFFFF, 0xFFFF};
    IOReg data2          = {0x0000, 0xFFFF, 0xFFFF};
    IOReg data3          = {0x0000, 0xFFFF, 0xFFFF};
    IOReg control        = {0x0000, 0x7FFF, 0x7FFF};
    IOReg send           = {0x0000, 0xFFFF, 0xFFFF};

    IOReg mode           = {0x0000, 0xC1FF, 0xC1FF};
    IOReg joybus_control = {0x0000, 0x0047, 0x0047};
    IOReg joybus_recv_l  = {0x0000, 0xFFFF, 0xFFFF};
    IOReg joybus_recv_h  = {0x0000, 0xFFFF, 0xFFFF};
    IOReg joybus_trans_l = {0x0000, 0xFFFF, 0xFFFF};
    IOReg joybus_trans_h = {0x0000, 0xFFFF, 0xFFFF};
    IOReg joybus_status  = {0x0000, 0x003A, 0x0030};

    static constexpr u16 joycnt_ack_mask   = 0x07;
    static constexpr u16 joycnt_irq_enable = 0x40;

    static constexpr u16 joystat_trans     = 0x8;
    static constexpr u16 joystat_recv      = 0x2;

private:
    //Core& core;
};

} // End namespace Gba
