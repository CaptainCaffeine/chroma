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

#include "gba/hardware/Timer.h"
#include "gba/core/Core.h"
#include "gba/core/Enums.h"
#include "gba/memory/Memory.h"

namespace Gba {

Timer::Timer(int _id, Core& _core)
        : id(_id)
        , core(_core) {
    if (id == 0) {
        // Cascade timing cannot be used for timer 0.
        control.read_mask = 0x00C3;
        control.write_mask = 0x00C3;
    }
}

void Timer::Tick(int cycles) {
    if (!TimerRunning() || CascadeEnabled()) {
        timer_clock += cycles;
        return;
    }

    while (cycles-- > 0) {
        timer_clock += 1;
        if (delay-- > 0) {
            continue;
        }

        if ((timer_clock & (CyclesPerTick() - 1)) == 0) {
            CounterTick();
        }
    }
}

int Timer::CyclesPerTick() const {
    const int prescaler_select = control & 0x0003;
    
    if (prescaler_select == 0) {
        return 1;
    } else {
        return 16 << (2 * prescaler_select);
    }
}

void Timer::CounterTick() {
    if (++counter == 0) {
        counter = reload;

        if (InterruptEnabled()) {
            core.mem->RequestInterrupt(Interrupt::Timer0 << id);
        }

        if (id < 3 && core.timers[id + 1].TimerRunning() && core.timers[id + 1].CascadeEnabled()) {
            core.timers[id + 1].CounterTick();
        }
    }
}

void Timer::WriteControl(const u16 data, const u16 mask) {
    bool was_stopped = !TimerRunning();
    control.Write(data, mask);

    if (was_stopped && TimerRunning()) {
        // The counter is reloaded when a timer is enabled.
        counter = reload;
        // Timers have a two cycle start up delay.
        delay = 2;
    }
}

} // End namespace Gba
