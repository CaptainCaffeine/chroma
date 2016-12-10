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

#include <array>

#include "common/CommonTypes.h"
#include "core/memory/Memory.h"

namespace Core {

class Timer {
public:
    Timer(Memory& memory);

    void UpdateTimer();

    // Debug Function
    void PrintRegisterState();
private:
    Memory& mem;

    const std::array<unsigned int, 4> select_div_bit {0x0200, 0x0008, 0x0020, 0x0080};

    bool prev_tima_inc = false;
    bool tima_overflow = false;
    bool tima_overflow_not_interrupted = false;
    u8 prev_tima_val = 0x00;

    unsigned int TACFrequency() const;
    bool TACEnable() const;
    bool TIMAIncWentLow(bool tima_inc) const;
    bool TIMAWasNotWritten(u8 current_tima_val) const;
    void LoadTMAIntoTIMA();
};

} // End namespace Core
