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
#include "gba/audio/Audio.h"

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
    if (TimerNotRunning()) {
        timer_clock += cycles;
        return;
    }

    if (delay > 0) {
        while (delay > 0 && cycles > 0) {
            delay -= 1;
            cycles -= 1;
            timer_clock += 1;
        }
    }

    while (cycles > 0) {
        if (cycles_per_tick == 1) {
            timer_clock += cycles;
            while (cycles-- > 0) {
                CounterTick();
            }
        } else {
            int remaining_cycles = cycles_per_tick - (timer_clock & (cycles_per_tick - 1));
            if (remaining_cycles > cycles) {
                timer_clock += cycles;
                return;
            }

            timer_clock += remaining_cycles;
            cycles -= remaining_cycles;

            CounterTick();
        }
    }
}

void Timer::CounterTick() {
    if (++counter == 0) {
        counter = reload;

        if (InterruptEnabled()) {
            core.mem->RequestInterrupt(Interrupt::Timer0 << id);
        }

        if (id < 3 && core.timers[id + 1].TimerEnabled() && core.timers[id + 1].CascadeEnabled()) {
            core.timers[id + 1].CounterTick();
        }

        if (id < 2) {
            for (int f = 0; f < 2; ++f) {
                if (id == core.audio->FifoTimerSelect(f)) {
                    core.audio->ConsumeSample(f);
                }
            }
        }
    }
}

void Timer::WriteControl(const u16 data, const u16 mask) {
    bool was_stopped = !TimerEnabled();
    control.Write(data, mask);

    if (was_stopped && TimerEnabled()) {
        // The counter is reloaded when a timer is enabled.
        counter = reload;
        // Timers have a two cycle start up delay.
        delay = 2;
    }

    const int prescaler_select = control & 0x0003;

    if (prescaler_select == 0) {
        cycles_per_tick = 1;
    } else {
        cycles_per_tick = 16 << (2 * prescaler_select);
    }

    if (id < 2) {
        for (int f = 0; f < 2; ++f) {
            if (id == core.audio->FifoTimerSelect(f)) {
                core.audio->fifos[f].samples_per_frame = (280896 / (cycles_per_tick * (0x1'0000 - reload))) * 5;
                if (core.audio->fifos[f].samples_per_frame <= 4 * 5) {
                    core.audio->fifos[f].sample_buffer.clear();
                }
            }
        }
    }
}

int Timer::NextEvent() const {
    if (!TimerEnabled()) {
        return 0;
    } else if (CascadeEnabled()) {
        return core.timers[id - 1].NextEvent();
    } else {
        int remaining_cycles_this_tick = cycles_per_tick - (timer_clock & (cycles_per_tick - 1));
        int remaining_ticks = (0xFFFF - counter) * cycles_per_tick;
        return delay + remaining_cycles_this_tick + remaining_ticks;
    }
}

} // End namespace Gba
