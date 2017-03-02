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

#include <chrono>
#include <vector>
#include <iosfwd> // for std::ofstream

#include "common/CommonTypes.h"

namespace Core {

class RTC {
public:
    u8 latch_last_value_written = 0xFF;

    void LatchCurrentTime();
    constexpr u8 GetFlags() const { return flags; }
    void SetFlags(u8 value);

    void LoadRTCData(const std::vector<u8>& save_game);
    void WriteRTCData(std::ofstream& save_file) const;

    template<typename T>
    constexpr u8 GetLatchedTime() const {
        return std::chrono::duration_cast<typename T::Duration>(latched_time).count() % T::mod;
    }

    template<typename T>
    constexpr void SetTime(u8 value) {
        typename T::Duration value_duration{value % T::mod};
        auto current_time_val{std::chrono::duration_cast<typename T::Duration>(CurrentInternalTime()) % T::mod};
        auto diff{value_duration - current_time_val};

        // std::chrono::time_point::operator-= isn't constexpr until C++17.
        reference_time = reference_time - diff;
    }

    template<typename T, int N>
    struct RTCDuration {
        using Duration = T;
        enum : int {mod = N};
    };

    using Seconds = RTCDuration<std::chrono::seconds, 60>;
    using Minutes = RTCDuration<std::chrono::minutes, 60>;
    using Hours = RTCDuration<std::chrono::hours, 24>;
    using Days = RTCDuration<std::chrono::duration<long, std::ratio<86400>>, 256>;
private:
    // The reference time is 0 seconds, 0 minutes, 0 hours, and 0 days in the MBC3 RTC time.
    std::chrono::time_point<std::chrono::steady_clock> reference_time{std::chrono::steady_clock::now()};
    std::chrono::time_point<std::chrono::steady_clock> halted_time{std::chrono::steady_clock::now()};
    std::chrono::seconds latched_time{0};

    // bit 0: MSB(it) of Day Counter
    // bit 6: Halt (0=Active, 1=Stop Timer)
    // bit 7: Day Counter Carry Bit
    u8 flags = 0x00;

    std::chrono::seconds CurrentInternalTime() const;

    void WriteRTCRegs(std::ofstream& save_file, std::chrono::seconds save_time) const;
    void WriteTimeStamp(std::ofstream& save_file) const;
    template<typename T>
    constexpr u32 BackCompatTimeValue(std::chrono::seconds save_time) const {
        return static_cast<u32>(std::chrono::duration_cast<typename T::Duration>(save_time).count() % T::mod);
    }
};

} // End namespace Core
