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

#include <fstream>

#include "core/memory/RTC.h"

namespace Core {

void RTC::LatchCurrentTime() {
    latched_time = CurrentInternalTime();

    auto days_msb = (std::chrono::duration_cast<Days::Duration>(latched_time).count() % 512) >> 8;
    if ((flags & 0x01) == 1 && days_msb == 0) {
        // Day counter overflowed, set carry bit.
        flags |= 0x80;
    }

    flags = (flags & 0xFE) | days_msb;
}

void RTC::SetFlags(u8 value) {
    // Clear unused bits.
    value &= 0xC1;

    // Handle the MSBit of the Days counter.
    Days::Duration diff{(value & 0x01) * 256 - (flags & 0x01) * 256};

    // std::chrono::time_point::operator-= isn't constexpr until C++17.
    reference_time = reference_time - diff;

    // Handle the RTC halt flag.
    if ((flags & 0x40) ^ (value & 0x40)) {
        if ((value & 0x40) == 1) {
            // Halt the RTC.
            halted_time = std::chrono::steady_clock::now();
        } else {
            // Unhalt the RTC.
            auto elapsed_time_halted = std::chrono::steady_clock::now() - halted_time;
            reference_time = reference_time + elapsed_time_halted;
        }
    }

    flags = value;
}

std::chrono::seconds RTC::CurrentInternalTime() const {
    if (flags & 0x40) {
        return std::chrono::duration_cast<std::chrono::seconds>(halted_time - reference_time);
    } else {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - reference_time);
    }
}

void RTC::LoadRTCData(const std::vector<u8>& save_game) {
    std::size_t save_size = save_game.size();

    // Load the latched time first.
    u8 saved_value = save_game[save_size - 28];
    SetTime<Seconds>(saved_value);

    saved_value = save_game[save_size - 24];
    SetTime<Minutes>(saved_value);

    saved_value = save_game[save_size - 20];
    SetTime<Hours>(saved_value);

    saved_value = save_game[save_size - 16];
    SetTime<Days>(saved_value);

    saved_value = save_game[save_size - 12];
    SetFlags(saved_value);

    LatchCurrentTime();

    // Load the previous internal time.
    saved_value = save_game[save_size - 48];
    SetTime<Seconds>(saved_value);

    saved_value = save_game[save_size - 44];
    SetTime<Minutes>(saved_value);

    saved_value = save_game[save_size - 40];
    SetTime<Hours>(saved_value);

    saved_value = save_game[save_size - 36];
    SetTime<Days>(saved_value);

    // No need to load flags a second time.

    // Get the timestamp of the last save.
    u64 save_timestamp = static_cast<u64>(save_game[save_size - 8]);
    save_timestamp |= static_cast<u64>(save_game[save_size - 7]) << 8;
    save_timestamp |= static_cast<u64>(save_game[save_size - 6]) << 16;
    save_timestamp |= static_cast<u64>(save_game[save_size - 5]) << 24;
    save_timestamp |= static_cast<u64>(save_game[save_size - 4]) << 32;
    save_timestamp |= static_cast<u64>(save_game[save_size - 3]) << 40;
    save_timestamp |= static_cast<u64>(save_game[save_size - 2]) << 48;
    save_timestamp |= static_cast<u64>(save_game[save_size - 1]) << 56;

    // Get elapsed time between last save and now.
    auto saved_system_time = std::chrono::system_clock::from_time_t(static_cast<std::time_t>(save_timestamp));
    auto elapsed_real_time = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                    std::chrono::system_clock::now() - saved_system_time
                                );

    reference_time -= elapsed_real_time;
}

void RTC::WriteRTCData(std::ofstream& save_file) const {
    // Since it's not actually part of the external RAM address space, the saved format of the RTC state is up
    // to the implementation. There is a somewhat-agreed upon format between emulators that was put in place
    // by either VBA or BGB ages ago.
    // The current/hidden values of the RTC registers are stored as little endian 32-bit words, followed by
    // the latched registers in the same format. After that, the current UNIX timestamp is stored as two
    // little-endian 32-bit words, with the low word first.
    // Not all emulators store the RTC state this way, but at least mGBA, BGB, and VBA-M do. Gambatte, GBE+, and
    // Higan do not.
    WriteRTCRegs(save_file, CurrentInternalTime());
    WriteRTCRegs(save_file, latched_time);
    WriteTimeStamp(save_file);
}

void RTC::WriteRTCRegs(std::ofstream& save_file, std::chrono::seconds save_time) const {
    u32 save_value = BackCompatTimeValue<Seconds>(save_time);
    save_file.write(reinterpret_cast<char*>(&save_value), sizeof(u32));

    save_value = BackCompatTimeValue<Minutes>(save_time);
    save_file.write(reinterpret_cast<char*>(&save_value), sizeof(u32));

    save_value = BackCompatTimeValue<Hours>(save_time);
    save_file.write(reinterpret_cast<char*>(&save_value), sizeof(u32));

    save_value = BackCompatTimeValue<Days>(save_time);
    save_file.write(reinterpret_cast<char*>(&save_value), sizeof(u32));

    save_value = static_cast<u32>(flags);
    save_file.write(reinterpret_cast<char*>(&save_value), sizeof(u32));
}

void RTC::WriteTimeStamp(std::ofstream& save_file) const {
    u64 timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    u32 save_value = timestamp & 0x00000000FFFFFFFF;
    save_file.write(reinterpret_cast<char*>(&save_value), sizeof(u32));
    save_value = (timestamp & 0xFFFFFFFF00000000) >> 32;
    save_file.write(reinterpret_cast<char*>(&save_value), sizeof(u32));
}

} // End namespace Core
