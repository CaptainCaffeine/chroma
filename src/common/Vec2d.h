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

#include <tuple>
#include <emmintrin.h>

#include "common/CommonTypes.h"

namespace Common {

class alignas(16) Vec2d {
public:
    Vec2d() = default;
    constexpr Vec2d(__m128d a)
            : vec(a) {}

    constexpr Vec2d(double a, double b)
            : vec{a, b} {}

    constexpr Vec2d(int a, int b)
            : vec{static_cast<double>(a), static_cast<double>(b)} {}

    __m128d vec;

    std::tuple<int, int> UnpackSamples() const {
        // Convert the packed doubles to packed 32-bit integers with rounding instead of truncation.
        // Truncation is the default behaviour for double->int conversion, but rounding is more accurate in this case.
        const __m128i vec_int = _mm_cvtpd_epi32(vec);
        // Extract the lower 64 bits containing the two ints into a normal register.
        const s64 packed_samples = _mm_cvtsi128_si64(vec_int);
        const int left_sample = packed_samples & 0xFFFF'FFFF;
        const int right_sample = (packed_samples >> 32);

        return {left_sample, right_sample};
    }

    // Binary arithmetic operators.
    Vec2d& operator+=(const Vec2d& rhs) {
        vec = _mm_add_pd(vec, rhs.vec);
        return *this;
    }

    Vec2d& operator-=(const Vec2d& rhs) {
        vec = _mm_sub_pd(vec, rhs.vec);
        return *this;
    }

    Vec2d& operator*=(const Vec2d& rhs) {
        vec = _mm_mul_pd(vec, rhs.vec);
        return *this;
    }

    Vec2d& operator/=(const Vec2d& rhs) {
        vec = _mm_div_pd(vec, rhs.vec);
        return *this;
    }

    Vec2d& operator&=(const Vec2d& rhs) {
        vec = _mm_and_pd(vec, rhs.vec);
        return *this;
    }

    Vec2d& operator|=(const Vec2d& rhs) {
        vec = _mm_or_pd(vec, rhs.vec);
        return *this;
    }

    Vec2d& operator^=(const Vec2d& rhs) {
        vec = _mm_xor_pd(vec, rhs.vec);
        return *this;
    }

    static void SetFlushToZero() {
        // This macro sets the FTZ bit in the MXCSR register, which causes all denormal results of SSE
        // operations to be flushed to zero.

        // The Butterworth lowpass IIR filter often generates denormal values in the z delays. These values are
        // far too small to impact the signal and they slow down the algorithm significantly, which is why we
        // flush denormal values to 0.
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    }
};

// Binary arithmetic operators.
inline Vec2d operator+(Vec2d lhs, const Vec2d& rhs) {
    lhs += rhs;
    return lhs;
}

inline Vec2d operator-(Vec2d lhs, const Vec2d& rhs) {
    lhs -= rhs;
    return lhs;
}

inline Vec2d operator*(Vec2d lhs, const Vec2d& rhs) {
    lhs *= rhs;
    return lhs;
}

inline Vec2d operator/(Vec2d lhs, const Vec2d& rhs) {
    lhs /= rhs;
    return lhs;
}

inline Vec2d operator&(Vec2d lhs, const Vec2d& rhs) {
    lhs &= rhs;
    return lhs;
}

inline Vec2d operator|(Vec2d lhs, const Vec2d& rhs) {
    lhs |= rhs;
    return lhs;
}

inline Vec2d operator^(Vec2d lhs, const Vec2d& rhs) {
    lhs ^= rhs;
    return lhs;
}

// Comparison operators.
inline Vec2d operator==(const Vec2d& lhs, const Vec2d& rhs) {
    return _mm_cmpeq_pd(lhs.vec, rhs.vec);
}

inline Vec2d operator!=(const Vec2d& lhs, const Vec2d& rhs) {
    return _mm_cmpneq_pd(lhs.vec, rhs.vec);
}

inline Vec2d operator<(const Vec2d& lhs, const Vec2d& rhs) {
    return _mm_cmplt_pd(lhs.vec, rhs.vec);
}

inline Vec2d operator>(const Vec2d& lhs, const Vec2d& rhs) {
    return _mm_cmpgt_pd(lhs.vec, rhs.vec);
}

inline Vec2d operator<=(const Vec2d& lhs, const Vec2d& rhs) {
    return _mm_cmple_pd(lhs.vec, rhs.vec);
}

inline Vec2d operator>=(const Vec2d& lhs, const Vec2d& rhs) {
    return _mm_cmpge_pd(lhs.vec, rhs.vec);
}

} // End namespace Common
