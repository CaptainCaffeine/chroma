// This file is a part of Chroma.
// Copyright (C) 2017-2018 Matthew Murray
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

#include <bitset>
#include <type_traits>
#include <array>

#include "common/CommonTypes.h"

constexpr u32 RotateRight(u32 value, unsigned int rotation) noexcept {
    return (value >> rotation) | (value << (-rotation & (sizeof(u32) * 8 - 1)));
}

template <typename T>
std::size_t Popcount(T value) noexcept {
    return std::bitset<sizeof(T) * 8>(value).count();
}

template <typename T>
constexpr std::make_signed_t<T> SignExtend(T value, unsigned int num_source_bits) noexcept {
    static_assert(std::is_unsigned<T>(), "Non-arithmetic or signed type passed to SignExtend.");

    const int shift_amount = sizeof(T) * 8 - num_source_bits;
    return static_cast<std::make_signed_t<T>>(value << shift_amount) >> shift_amount;
}

constexpr unsigned int LowestSetBit(u32 value) noexcept {
    constexpr std::array<unsigned int, 32> DeBruijnHashTable{{
        0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9
    }};

    return DeBruijnHashTable[((value & -value) * 0x077CB531) >> 27];
}

constexpr unsigned int HighestSetBit(u32 value) noexcept {
    constexpr std::array<unsigned int, 32> DeBruijnHashTable{{
        0, 9,  1,  10, 13, 21, 2,  29, 11, 14, 16, 18, 22, 25, 3, 30,
        8, 12, 20, 28, 15, 17, 24, 7,  19, 27, 23, 6,  26, 5,  4, 31
    }};

    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;

    return DeBruijnHashTable[(value * 0x07C4ACDD) >> 27];
}

template<class ByteIter>
constexpr u32 Fnv1aHash(ByteIter addr, ByteIter end) noexcept {
    constexpr u32 fnv_prime = 0x01000193;
    u32 hash_val = 0x811C9DC5;

    for (; addr != end; ++addr) {
        hash_val ^= *addr;
        hash_val *= fnv_prime;
    }

    return hash_val;
}
