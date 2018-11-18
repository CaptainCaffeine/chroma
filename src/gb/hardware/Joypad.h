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

class GameBoy;

class Joypad {
public:
    explicit Joypad(GameBoy& _gameboy)
            : gameboy(_gameboy) {}

    enum Button : u8 {Right  = 0x01,
                      Left   = 0x02,
                      Up     = 0x04,
                      Down   = 0x08,
                      A      = 0x10,
                      B      = 0x20,
                      Select = 0x40,
                      Start  = 0x80};

    void UpdateJoypad();
    void Press(Button button, bool pressed) {
        if (pressed) {
            // When pressing a directional button, record if the opposite direction was currently pressed and
            // unpress it.
            if (button == Button::Up || button == Button::Right) {
                was_unset |= ~button_states & (button << 1);
                button_states |= button << 1;
            } else if (button == Button::Down || button == Button::Left) {
                was_unset |= ~button_states & (button >> 1);
                button_states |= button >> 1;
            }

            button_states &= ~button;
        } else {
            // When releasing a directional button, re-press the opposite direction if we unpressed it earlier and
            // the player hasn't unpressed it yet.
            if (button == Button::Up || button == Button::Right) {
                was_unset &= ~button;
                button_states &= ~(was_unset & (button << 1));
                was_unset &= ~(button << 1);
            } else if (button == Button::Down || button == Button::Left) {
                was_unset &= ~button;
                button_states &= ~(was_unset & (button >> 1));
                was_unset &= ~(button >> 1);
            }

            button_states |= button;
        }
    }

    bool JoypadPress() const { return (p1 & 0x0F) != 0x0F; }

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
    GameBoy& gameboy;

    u8 button_states = 0xFF;
    u8 was_unset = 0x00;

    bool prev_interrupt_signal = false;

    bool ButtonKeysSelected() const { return !(p1 & 0x20); }
    bool DirectionKeysSelected() const { return !(p1 & 0x10); }
};

} // End namespace Gb
