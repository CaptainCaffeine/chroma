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

#include <array>

#include "common/CommonTypes.h"

namespace Gb {

class GameBoy;

class Timer {
public:
    explicit Timer(GameBoy& _gameboy)
            : gameboy(_gameboy) {}

    void UpdateTimer();

    // ******** Timer I/O registers ********
    // DIV register: 0xFF04
    u16 divider = 0x0000;
    // TIMA register: 0xFF05
    u8 tima = 0x00;
    // TMA register: 0xFF06
    u8 tma = 0x00;
    // TAC register: 0xFF07
    //     bit 2: Timer Enable
    //     bits 1&0: Main Frequency Divider (0=every 1024 cycles, 1=16 cycles, 2=64 cycles, 3=256 cycles)
    u8 tac = 0x00;
private:
    GameBoy& gameboy;

    bool prev_tima_inc = false;
    bool tima_overflow = false;
    bool tima_overflow_not_interrupted = false;
    u8 prev_tima_val = 0x00;

    const std::array<unsigned int, 4> select_div_bit{{0x0200, 0x0008, 0x0020, 0x0080}};

    bool DivFrequencyBitSet() const { return select_div_bit[tac & 0x03] & divider; }
    bool TimerEnabled() const { return tac & 0x04; }
};

} // End namespace Gb
