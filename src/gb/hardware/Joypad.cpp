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

#include "gb/hardware/Joypad.h"
#include "gb/memory/Memory.h"

namespace Gb {

void Joypad::UpdateJoypad() {
    // Set all button inputs high.
    p1 |= 0x0F;

    if (ButtonKeysSelected()) {
        p1 = (p1 & 0xF0) | (button_states >> 4);
    }

    if (DirectionKeysSelected()) {
        // AND button states with p1 in case the button keys are also selected.
        p1 = (p1 & 0xF0) | (button_states & p1);
    }

    bool interrupt_signal = (p1 & 0x0F) == 0x0F;

    bool signal_went_low = !interrupt_signal && prev_interrupt_signal;
    if (signal_went_low) {
        mem->RequestInterrupt(Interrupt::Joypad);
    }

    prev_interrupt_signal = interrupt_signal;
}

} // End namespace Gb
