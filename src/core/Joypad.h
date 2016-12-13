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

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

namespace Core {

class Memory;

class Joypad {
public:
    const u8 start = 0x80, select = 0x40, b = 0x20, a = 0x10;
    const u8 down = 0x08, up = 0x04, left = 0x02, right = 0x01;

    // Start, Select, B, A, Down, Up, Left, Right.
    u8 button_states = 0xFF;

    void StartPressed(bool val) { (val) ? (button_states &= ~start) : (button_states |= start); }
    void SelectPressed(bool val) { (val) ? (button_states &= ~select) : (button_states |= select); }
    void BPressed(bool val) { (val) ? (button_states &= ~b) : (button_states |= b); }
    void APressed(bool val) { (val) ? (button_states &= ~a) : (button_states |= a); }

    void DownPressed(bool val) { (val) ? (button_states &= ~down) : (button_states |= down); }
    void UpPressed(bool val) { (val) ? (button_states &= ~up) : (button_states |= up); }
    void LeftPressed(bool val) { (val) ? (button_states &= ~left) : (button_states |= left); }
    void RightPressed(bool val) { (val) ? (button_states &= ~right) : (button_states |= right); }

    void UpdateJoypad();

    void LinkToMemory(Memory* memory) { mem = memory; }

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
    Memory* mem;

    bool prev_interrupt_signal = false;

    bool ButtonKeysSelected() { return !(p1 & 0x20); }
    bool DirectionKeysSelected() { return !(p1 & 0x10); }
};

} // End namespace Core
