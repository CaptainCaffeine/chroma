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

#include "common/CommonTypes.h"

namespace Gba {

struct IOReg {
    u16 v;
    u16 read_mask;
    u16 write_mask;

    u16 Read() const { return v & read_mask; }
    void Write(u16 data, u16 mask_8bit) { v = (v & ~(write_mask & mask_8bit)) | (data & write_mask); }
    void Clear(u16 data) { v &= ~(data & write_mask); }

    // Assignment operator.
    IOReg& operator=(IOReg rhs) {
        v = rhs.v;
        return *this;
    }

    IOReg& operator=(int rhs) {
        v = rhs;
        return *this;
    }

    // Conversion operator.
    operator int() const {
        return v;
    }

    // Unary operators.
    IOReg& operator++() {
        ++v;
        return *this;
    }

    IOReg operator++(int) {
        IOReg temp{*this};
        operator++();
        return temp;
    }

    IOReg& operator--() {
        --v;
        return *this;
    }

    IOReg operator--(int) {
        IOReg temp{*this};
        operator--();
        return temp;
    }

    // Binary arithmetic operators with another IOReg.
    IOReg& operator+=(const IOReg& rhs) {
        v += rhs.v;
        return *this;
    }

    IOReg& operator-=(const IOReg& rhs) {
        v -= rhs.v;
        return *this;
    }

    IOReg& operator*=(const IOReg& rhs) {
        v *= rhs.v;
        return *this;
    }

    IOReg& operator/=(const IOReg& rhs) {
        v /= rhs.v;
        return *this;
    }

    IOReg& operator&=(const IOReg& rhs) {
        v &= rhs.v;
        return *this;
    }

    IOReg& operator|=(const IOReg& rhs) {
        v |= rhs.v;
        return *this;
    }

    IOReg& operator^=(const IOReg& rhs) {
        v ^= rhs.v;
        return *this;
    }

    IOReg& operator<<=(const IOReg& rhs) {
        v <<= rhs.v;
        return *this;
    }

    IOReg& operator>>=(const IOReg& rhs) {
        v >>= rhs.v;
        return *this;
    }

    // Binary arithmetic operators with an int.
    IOReg& operator+=(int rhs) {
        v += rhs;
        return *this;
    }

    IOReg& operator-=(int rhs) {
        v -= rhs;
        return *this;
    }

    IOReg& operator*=(int rhs) {
        v *= rhs;
        return *this;
    }

    IOReg& operator/=(int rhs) {
        v /= rhs;
        return *this;
    }

    IOReg& operator&=(int rhs) {
        v &= rhs;
        return *this;
    }

    IOReg& operator|=(int rhs) {
        v |= rhs;
        return *this;
    }

    IOReg& operator^=(int rhs) {
        v ^= rhs;
        return *this;
    }

    IOReg& operator<<=(int rhs) {
        v <<= rhs;
        return *this;
    }

    IOReg& operator>>=(int rhs) {
        v >>= rhs;
        return *this;
    }
};

// Binary arithmetic operators with another IOReg.
inline IOReg operator+(IOReg lhs, const IOReg& rhs) {
    lhs += rhs;
    return lhs;
}

inline IOReg operator-(IOReg lhs, const IOReg& rhs) {
    lhs -= rhs;
    return lhs;
}

inline IOReg operator*(IOReg lhs, const IOReg& rhs) {
    lhs *= rhs;
    return lhs;
}

inline IOReg operator/(IOReg lhs, const IOReg& rhs) {
    lhs /= rhs;
    return lhs;
}

inline IOReg operator&(IOReg lhs, const IOReg& rhs) {
    lhs &= rhs;
    return lhs;
}

inline IOReg operator|(IOReg lhs, const IOReg& rhs) {
    lhs |= rhs;
    return lhs;
}

inline IOReg operator^(IOReg lhs, const IOReg& rhs) {
    lhs ^= rhs;
    return lhs;
}

inline IOReg operator<<(IOReg lhs, const IOReg& rhs) {
    lhs <<= rhs;
    return lhs;
}

inline IOReg operator>>(IOReg lhs, const IOReg& rhs) {
    lhs >>= rhs;
    return lhs;
}

// Binary arithmetic operators with an int not needed due to implicit conversion to int.

// Comparison operators.
inline bool operator==(const IOReg& lhs, const IOReg& rhs) {
    return lhs.v == rhs.v;
}

inline bool operator!=(const IOReg& lhs, const IOReg& rhs) {
    return !operator==(lhs, rhs);
}

inline bool operator<(const IOReg& lhs, const IOReg& rhs) {
    return lhs.v < rhs.v;
}

inline bool operator>(const IOReg& lhs, const IOReg& rhs) {
    return operator<(rhs, lhs);
}

inline bool operator<=(const IOReg& lhs, const IOReg& rhs) {
    return !operator>(lhs, rhs);
}

inline bool operator>=(const IOReg& lhs, const IOReg& rhs) {
    return !operator<(lhs, rhs);
}

// Comparison operators with an int not needed due to implicit conversion to int.

} // End namespace Gba
