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

#include "core/Serial.h"
#include "core/memory/Memory.h"

namespace Core {

void Serial::UpdateSerial() {
    // Serial clock advances with the system clock.
    serial_clock += 4;

    // Check if a transfer has been initiated.
    if (bits_to_shift == 0 && (serial_control & 0x80)) {
        bits_to_shift = 8;
    }

    // A falling edge on the internal transfer signal causes a bit to be shifted out/in.
    if (bits_to_shift > 0 && !transfer_signal && prev_transfer_signal) {
        ShiftSerialBit();
    }

    prev_transfer_signal = transfer_signal;

    bool serial_inc = (serial_clock & SelectClockBit()) && UsingInternalClock();

    // When using the internal clock, a falling edge on bit 7 of the serial clock causes the internal transfer
    // signal to be toggled.
    if (!serial_inc && prev_inc) {
        transfer_signal = !transfer_signal;
    }

    prev_inc = serial_inc;
}

void Serial::ShiftSerialBit() {
    // Shift the most significant bit out of SB.
    serial_data <<= 1;

    // Since we are always emulating a disconnected serial port at the moment, place a 1 in the least significant
    // bit of SB.
    serial_data |= 0x01;

    if (--bits_to_shift == 0) {
        // The transfer has completed.
        serial_control &= 0x7F;
        mem->RequestInterrupt(Interrupt::Serial);
    }
}

u8 Serial::SelectClockBit() const {
    // In CBG mode, bit 1 of SC can be used to set the speed of the serial transfer. The transfer runs at the usual 
    // speed (using bit 7 of the serial clock) if it's 0, and runs fast (using bit 2 of the serial clock) if it's 1.
    // In DMG mode, bit 1 of SC returns 1 even though the transfer runs at the usual speed.
    if (mem->game_mode == GameMode::CGB) {
        return (serial_control & 0x02) ? 0x04 : 0x80;
    } else {
        return 0x80;
    }
}

} // End namespace Core
