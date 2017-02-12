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

#include "common/CommonTypes.h"

namespace Core {

class RTC {
public:
    u8 latch_last_value_written = 0xFF;

    RTC(std::chrono::time_point<std::chrono::steady_clock> initial_time) : reference_time(initial_time) {}
    RTC() = default;

    constexpr void LatchCurrentTime() {
        latched_time = CurrentInternalTime();

        auto days_msb = (std::chrono::duration_cast<Days::Duration>(latched_time).count() % 512) >> 8;
        if ((flags & 0x01) == 1 && days_msb == 0) {
            // Day counter overflowed, set carry bit.
            flags |= 0x80;
        }

        flags = (flags & 0xFE) | days_msb;
    }

    template<typename T>
    constexpr u8 GetLatchedTime() const {
        return std::chrono::duration_cast<typename T::Duration>(latched_time).count() % T::mod;
    }

    constexpr u8 GetFlags() const {
        return flags;
    }

    template<typename T>
    constexpr void SetTime(u8 value) {
        typename T::Duration value_duration{value % T::mod};
        auto current_time_val{std::chrono::duration_cast<typename T::Duration>(CurrentInternalTime()) % T::mod};
        auto diff{value_duration - current_time_val};

        // std::chrono::time_point::operator-= isn't constexpr until C++17.
        reference_time = reference_time - diff;
    }

    constexpr void SetFlags(u8 value) {
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

    template<typename T, int N>
    struct RTCDuration {
        using Duration = T;
        constexpr static int mod{N};
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

    constexpr std::chrono::seconds CurrentInternalTime() const {
        if (flags & 0x40) {
            return std::chrono::duration_cast<std::chrono::seconds>(halted_time - reference_time);
        } else {
            return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - reference_time);
        }
    }
};

// Namespace-level definition of mod needed to avoid ODR-usage linker errors.
template<typename T, int N> constexpr int RTC::RTCDuration<T, N>::mod;

} // End namespace Core
