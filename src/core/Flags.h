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

#include "common/CommonTypes.h"

namespace Core {

class Flags {
private:
    const u8 zero  = 0x80;
    const u8 sub   = 0x40;
    const u8 half  = 0x20;
    const u8 carry = 0x10;
public:
    // bits is public so the flags can be easily popped on/pushed off the stack.
    u8 bits;

    // Setters
    void SetZero(bool val)  { (val) ? (bits |= zero)  : (bits &= ~zero);  }
    void SetSub(bool val)   { (val) ? (bits |= sub)   : (bits &= ~sub);   }
    void SetHalf(bool val)  { (val) ? (bits |= half)  : (bits &= ~half);  }
    void SetCarry(bool val) { (val) ? (bits |= carry) : (bits &= ~carry); }

    // Getters
    u8 Zero()  const { return (bits & zero)  >> 7; }
    u8 Sub()   const { return (bits & sub)   >> 6; }
    u8 Half()  const { return (bits & half)  >> 5; }
    u8 Carry() const { return (bits & carry) >> 4; }
};

} // End namespace Core
