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

#include "core/memory/Memory.h"

namespace Core {

class Serial {
public:
    Serial(Memory& memory);

    void UpdateSerial();
private:
    Memory& mem;

    u8 serial_clock;
    int bits_to_shift = 0;
    bool prev_inc = false;

    bool transfer_signal = false;
    bool prev_transfer_signal = false;

    void ShiftSerialBit();
    bool UsingInternalClock() const { return mem.ReadMem8(0xFF02) & 0x01; }
    u8 SelectClockBit() const;
};

} // End namespace Core
