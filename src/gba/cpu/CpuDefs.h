// This file is a part of Chroma.
// Copyright (C) 2017-2019 Matthew Murray
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

namespace Gba {

using Arm = u32;
using Thumb = u16;

using Reg = u32;
static constexpr Reg sp = 13, lr = 14, pc = 15;

enum class Condition {Equal         = 0b0000,
                      NotEqual      = 0b0001,
                      CarrySet      = 0b0010,
                      CarryClear    = 0b0011,
                      Minus         = 0b0100,
                      Plus          = 0b0101,
                      OverflowSet   = 0b0110,
                      OverflowClear = 0b0111,
                      Higher        = 0b1000,
                      LowerSame     = 0b1001,
                      GreaterEqual  = 0b1010,
                      LessThan      = 0b1011,
                      GreaterThan   = 0b1100,
                      LessEqual     = 0b1101,
                      Always        = 0b1110};

enum class ShiftType {LSL = 0,
                      LSR = 1,
                      ASR = 2,
                      ROR = 3,
                      RRX = 4};

struct ImmediateShift {
    ShiftType type;
    u32 imm;
};

} // End namespace Gba
