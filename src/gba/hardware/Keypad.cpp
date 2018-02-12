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

#include "gba/hardware/Keypad.h"
#include "gba/core/Core.h"
#include "gba/core/Enums.h"
#include "gba/memory/Memory.h"

namespace Gba {

Keypad::Keypad(Core& _core)
        : core(_core) {}

void Keypad::CheckKeypadInterrupt() {
    bool interrupt_requested = false;
    if (control & irq_enable) {
        if (control & irq_condition) {
            // All of the selected buttons must be pressed.
            interrupt_requested = (SelectedIrqButtons() & ~input) == SelectedIrqButtons();
        } else {
            // At least one of the selected buttons must be pressed.
            interrupt_requested = (SelectedIrqButtons() & ~input) != 0;
        }
    }

    // Don't request an interrupt multiple times for the same key press.
    if (interrupt_requested && !already_requested) {
        core.mem->RequestInterrupt(Interrupt::Keypad);
    }

    already_requested = interrupt_requested;
}

} // End namespace Gba
