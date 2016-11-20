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

#include "core/Timer.h"

namespace Core {

Timer::Timer(Memory& memory) : mem(memory) {}

void Timer::UpdateTimer() {
    // Increment the timer subsystem by 4 cycles.
 
    // DIV increments by 1 each clock cycle.
    mem.IncrementDIV(4);

    // If TIMA overflowed during the previous machine cycle, load TMA into it and set the timer interrupt.
    // If TIMA was written during this machine cycle, the value is overwritten with TMA.
    if (tima_overflow) {
        LoadTMAIntoTIMA();
        mem.WriteMem8(0xFF0F, mem.ReadMem8(0xFF0F) | 0x04);

        tima_overflow = false;
    }

    // TIMA conceptually increases once every specified number of cycles while the timer enable bit in TAC is set.
    // This is accomplished by testing if either bit 9, 7, 5, or 3 of the DIV register goes from 1 to 0; the 
    // particular bit depends on the frequency set in TAC.
    // In reality, the bit from DIV is ANDed with the timer enable bit *before* it goes through the falling edge
    // detector. This can cause the timer to increase in several unexpected situations. The easiest way to
    // handle this behaviour is to LLE what we know of the timer increment circuit in the Game Boy (courtesy 
    // of AntonioND's thorough timing documentation) instead of attempting to HLE each edge case. Unfortunately,
    // not enough is known about the rest of the timer circuitry (overflow, write priorities) to attempt LLE.

    bool div_tick_bit = select_div_bit[TACFrequency()] & mem.ReadDIV();
    bool tima_inc = div_tick_bit && TACEnable();
    u8 tima_val = mem.ReadMem8(0xFF05);

    if (TIMAIncWentLow(tima_inc)) {
        // When TIMA overflows, there is a delay of one machine cycle before it is loaded with TMA and the timer 
        // interrupt is triggered.
        // If TIMA is written to on a machine cycle where it would have overflowed, the overflow procedure is aborted.
        tima_overflow = (tima_val == 0xFF && TIMAWasNotWritten(tima_val));

        mem.WriteMem8(0xFF05, ++tima_val);
    }

    prev_tima_val = tima_val;
    prev_tima_inc = tima_inc;
}

unsigned int Timer::TACFrequency() const {
    return mem.ReadMem8(0xFF07) & 0x03;
}

bool Timer::TACEnable() const {
    return mem.ReadMem8(0xFF07) & 0x04;
}

bool Timer::TIMAIncWentLow(bool tima_inc) const {
    return !tima_inc && prev_tima_inc;
}

bool Timer::TIMAWasNotWritten(u8 current_tima_val) const {
    return prev_tima_val == current_tima_val;
}

void Timer::LoadTMAIntoTIMA() {
    mem.WriteMem8(0xFF05, mem.ReadMem8(0xFF06));
}

} // End namespace Core
