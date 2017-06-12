// This file is a part of Chroma.
// Copyright (C) 2016-2017 Matthew Murray
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
#include "common/CommonEnums.h"

namespace Core {

class Memory;

class Serial {
public:
    void UpdateSerial();

    constexpr void InitSerialClock(u8 init_val) { serial_clock = init_val; }
    constexpr void LinkToMemory(Memory* memory) { mem = memory; }

    // ******** Serial I/O registers ********
    // SB register: 0xFF01
    u8 serial_data = 0x00;
    // SC register: 0xFF02
    //     bit 7: Transfer Start Flag
    //     bit 1: Transfer Speed (0=Normal, 1=Fast) (CGB Only)
    //     bit 0: Shift Clock (0=External Clock, 1=Internal Clock 8192Hz)
    u8 serial_control = 0x00;
private:
    Memory* mem = nullptr;

    u8 serial_clock = 0x00;
    int bits_to_shift = 0;
    bool prev_inc = false;

    bool transfer_signal = false;
    bool prev_transfer_signal = false;

    void ShiftSerialBit();
    u8 SelectClockBit() const;
    constexpr bool UsingInternalClock() const { return serial_control & 0x01; }
};

} // End namespace Core
