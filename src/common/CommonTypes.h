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

#include <cstdint>

// Unsigned specific bit-width types.
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// Signed specific bit-width types.
using s8  = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using Arm = u32;
using Thumb = u16;

struct IOReg {
    u16 v;
    u16 read_mask;
    u16 write_mask;

    u16 Read() const { return v & read_mask; }
    void Write(u16 data, u16 mask_8bit) { v = (v & ~(write_mask & mask_8bit)) | (data & write_mask); }
    void Clear(u16 data) { v &= ~(data & write_mask); }
};
