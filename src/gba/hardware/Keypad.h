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

class Keypad {
public:
    Keypad(Core& _core);

    IOReg input   = {0x03FF, 0x03FF, 0x0000};
    IOReg control = {0x0000, 0xC3FF, 0xC3FF};

    enum Button : u16 {A      = 0x0001,
                       B      = 0x0002,
                       Select = 0x0004,
                       Start  = 0x0008,
                       Right  = 0x0010,
                       Left   = 0x0020,
                       Up     = 0x0040,
                       Down   = 0x0080,
                       R      = 0x0100,
                       L      = 0x0200};

    void CheckKeypadInterrupt();
    void Press(Button button, bool pressed) {
        if (pressed) {
            // When pressing a directional button, record if the opposite direction was currently pressed and
            // unpress it.
            if (button == Button::Up || button == Button::Right) {
                was_unset |= ~input & (button << 1);
                input |= button << 1;
            } else if (button == Button::Down || button == Button::Left) {
                was_unset |= ~input & (button >> 1);
                input |= button >> 1;
            }

            input &= ~button;
        } else {
            // When releasing a directional button, re-press the opposite direction if we unpressed it earlier and
            // the player hasn't unpressed it yet.
            if (button == Button::Up || button == Button::Right) {
                was_unset &= ~button;
                input &= ~(was_unset & (button << 1));
                was_unset &= ~(button << 1);
            } else if (button == Button::Down || button == Button::Left) {
                was_unset &= ~button;
                input &= ~(was_unset & (button >> 1));
                was_unset &= ~(button >> 1);
            }

            input |= button;
        }
    }

private:
    Core& core;

    bool already_requested = false;
    u16 was_unset = 0x00;

    // Control flags
    static constexpr u16 irq_enable = 0x4000;
    static constexpr u16 irq_condition = 0x8000;
    u16 SelectedIrqButtons() const { return control & 0x03FF; }
};

} // End namespace Gba
