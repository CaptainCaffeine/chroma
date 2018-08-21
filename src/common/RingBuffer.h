// This file is a part of Chroma.
// Copyright (C) 2018 Matthew Murray
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
#include <algorithm>

#include "common/CommonTypes.h"

namespace Common {

template<typename T, int N>
class RingBuffer {
public:
    constexpr int Size() const { return size; }
    constexpr T Read() const { return ring_buffer[read_index]; }

    constexpr T PopFront() {
        const T value = ring_buffer[read_index++];
        read_index %= length;
        size -= 1;

        return value;
    }

    constexpr void PushBack(T data) {
        ring_buffer[write_index++] = std::move(data);
        write_index %= length;
        size += 1;
    }

    constexpr void Reset() {
        std::fill(ring_buffer.begin(), ring_buffer.end(), T{});
        read_index = 0;
        write_index = 0;
        size = 0;
    }

private:
    static constexpr int length = N;
    std::array<T, length> ring_buffer{};
    int read_index = 0;
    int write_index = 0;
    int size = 0;
};

} // End namespace Common
