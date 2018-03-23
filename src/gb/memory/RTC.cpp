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

#include <fmt/format.h>

#include "gb/memory/RTC.h"

namespace Gb {

RTC::RTC(std::vector<u8>& save_game) {
    if ((save_game.size() % 0x400) != 0x30) {
        fmt::print("No RTC save data found. RTC initialized to default time.\n");
    } else {
        LoadRTCData(save_game);
        save_game.erase(save_game.cend() - 0x30, save_game.cend());
    }
}

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

    reference_time -= diff;

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
    const std::size_t save_size = save_game.size();

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

void RTC::AppendRTCData(std::vector<u8>& save_game) const {
    // Since it's not actually part of the external RAM address space, the saved format of the RTC state is up
    // to the implementation. There is a somewhat-agreed upon format between emulators that was put in place
    // by either VBA or BGB ages ago.
    // The current/hidden values of the RTC registers are stored as little endian 32-bit words, followed by
    // the latched registers in the same format. After that, the current UNIX timestamp is stored as two
    // little-endian 32-bit words, with the low word first.
    // Not all emulators store the RTC state this way, but at least mGBA, BGB, and VBA-M do. Gambatte, GBE+, and
    // Higan do not.
    AppendRTCRegs(save_game, CurrentInternalTime());
    AppendRTCRegs(save_game, latched_time);
    AppendTimeStamp(save_game);
}

void RTC::AppendRTCRegs(std::vector<u8>& save_file, std::chrono::seconds save_time) const {
    PushBackAs32Bits(save_file, GetTimeValue<Seconds>(save_time));
    PushBackAs32Bits(save_file, GetTimeValue<Minutes>(save_time));
    PushBackAs32Bits(save_file, GetTimeValue<Hours>(save_time));
    PushBackAs32Bits(save_file, GetTimeValue<Days>(save_time));
    PushBackAs32Bits(save_file, flags);
}

void RTC::AppendTimeStamp(std::vector<u8>& save_file) const {
    const u64 timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    save_file.push_back(static_cast<u8>(timestamp));
    save_file.push_back(static_cast<u8>(timestamp >> 8));
    save_file.push_back(static_cast<u8>(timestamp >> 16));
    save_file.push_back(static_cast<u8>(timestamp >> 24));
    save_file.push_back(static_cast<u8>(timestamp >> 32));
    save_file.push_back(static_cast<u8>(timestamp >> 40));
    save_file.push_back(static_cast<u8>(timestamp >> 48));
    save_file.push_back(static_cast<u8>(timestamp >> 56));
}

void RTC::PushBackAs32Bits(std::vector<u8>& save_file, const u8 value) const {
    save_file.push_back(value);
    save_file.push_back(0);
    save_file.push_back(0);
    save_file.push_back(0);
}

} // End namespace Gb
