// This file is a part of Chroma.
// Copyright (C) 2016-2018 Matthew Murray
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
#include "gb/core/Enums.h"

namespace Gb {

class Memory;

class Joypad {
public:
    enum Button : u8 {Right  = 0x01,
                      Left   = 0x02,
                      Up     = 0x04,
                      Down   = 0x08,
                      A      = 0x10,
                      B      = 0x20,
                      Select = 0x40,
                      Start  = 0x80};

    void UpdateJoypad();
    void Press(Button button, bool pressed) { (pressed) ? (button_states &= ~button) : (button_states |= button); }

    constexpr void LinkToMemory(Memory* memory) { mem = memory; }
    constexpr bool JoypadPress() const { return (p1 & 0x0F) != 0x0F; }

    // ******** Joypad I/O register ********
    // P1 register: 0xFF00
    //     bit 5: P15 Select Button Keys (0=Select)
    //     bit 4: P14 Select Direction Keys (0=Select)
    //     bit 3: P13 Input Down or Start (0=Pressed)
    //     bit 2: P12 Input Up or Select (0=Pressed)
    //     bit 1: P11 Input Left or B (0=Pressed)
    //     bit 0: P10 Input Right or A (0=Pressed)
    u8 p1 = 0x00;
private:
    Memory* mem = nullptr;

    // Start, Select, B, A, Down, Up, Left, Right.
    u8 button_states = 0xFF;

    bool prev_interrupt_signal = false;

    constexpr bool ButtonKeysSelected() const { return !(p1 & 0x20); }
    constexpr bool DirectionKeysSelected() const { return !(p1 & 0x10); }
};

} // End namespace Gb
