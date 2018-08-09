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
    while (delay > 0 && cycles > 0) {
        delay -= 1;
        cycles -= 1;
        timer_clock += 1;
    }

    if (cycles_per_tick == 1) {
        timer_clock += cycles;

        while (cycles > 0) {
            const int remaining_ticks = 0x1'0000 - counter;
            if (remaining_ticks > cycles) {
                counter += cycles;
                return;
            }

            // Set the counter so it overflows in CounterTick().
            counter += remaining_ticks - 1;
            cycles -= remaining_ticks;

            CounterTick();
        }
    } else {
        while (cycles > 0) {
            const int remaining_cycles = cycles_per_tick - (timer_clock & (cycles_per_tick - 1));
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
    Tick(core.timer_cycle_counter[id]);

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

    core.timer_cycle_counter[id] = 0;
    core.next_timer_event_cycles[id] = NextEvent();
}

u16 Timer::ReadCounter() {
    Tick(core.timer_cycle_counter[id]);
    core.timer_cycle_counter[id] = 0;
    core.next_timer_event_cycles[id] = NextEvent();

    return counter;
}

void Timer::WriteReload(const u16 data, const u16 mask) {
    Tick(core.timer_cycle_counter[id]);
    core.timer_cycle_counter[id] = 0;
    core.next_timer_event_cycles[id] = NextEvent();

    reload.Write(data, mask);
}

int Timer::NextEvent() const {
    if (!TimerEnabled()) {
        return 0;
    } else if (CascadeEnabled()) {
        return core.timers[id - 1].NextEvent();
    } else {
        const int remaining_cycles_this_tick = cycles_per_tick - (timer_clock & (cycles_per_tick - 1));
        const int remaining_ticks = (0xFFFF - counter) * cycles_per_tick;
        if (cycles_per_tick == 1) {
            return delay + remaining_cycles_this_tick + remaining_ticks;
        } else {
            // The timer_clock continues to increment during the delay, so it doesn't delay when the next tick
            // happens, it just prevents a tick from occuring in the next two cycles.
            return remaining_cycles_this_tick + remaining_ticks;
        }
    }
}

} // End namespace Gba
