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

class alignas(16) Vec4f {
public:
    constexpr Vec4f() = default;
    constexpr Vec4f(__m128 a)
            : vec(a) {}

    constexpr Vec4f(float a, float b, float c = 0.0f, float d = 0.0f)
            : vec{a, b, c, d} {}

    constexpr Vec4f(int a, int b, int c = 0, int d = 0)
            : vec{static_cast<float>(a), static_cast<float>(b), static_cast<float>(c), static_cast<float>(d)} {}

    __m128 vec{0};

    std::tuple<int, int> UnpackSamples(bool low_samples) const {
        // Convert the packed floats to packed 32-bit integers with rounding instead of truncation.
        // Truncation is the default behaviour for float->int conversion, but rounding is more accurate in this case.
        __m128i vec_int;
        if (low_samples) {
            vec_int = _mm_cvtps_epi32(vec);
        } else {
            // Move the high samples into the bottom half of the register before converting.
            vec_int = _mm_cvtps_epi32(_mm_movehl_ps(vec, vec));
        }

        // Extract the lower 64 bits containing the stereo sample into a normal register.
        const s64 packed_samples = _mm_cvtsi128_si64(vec_int);
        const int left_sample = packed_samples & 0xFFFF'FFFF;
        const int right_sample = (packed_samples >> 32);

        return {left_sample, right_sample};
    }

    static Vec4f Combine(Vec4f low_source, Vec4f high_source) {
        // Copy the low half of low_source to the low half of the destination,
        // and the high half of high_source into the high half of the destination.
        return {_mm_shuffle_ps(low_source.vec, high_source.vec, (0 << 0) | (1 << 2) | (2 << 4) | (3 << 6))};
    }

    static Vec4f CombineAndSwap(Vec4f low_source, Vec4f high_source) {
        // Copy the low half of low_source to the high half of the destination,
        // and the high half of high_source into the low half of the destination.
        return {_mm_shuffle_ps(high_source.vec, low_source.vec, (2 << 0) | (3 << 2) | (0 << 4) | (1 << 6))};
    }

    static Vec4f Swap(Vec4f source) {
        // Swap the low and high halves of the Vec4f.
        return {_mm_shuffle_ps(source.vec, source.vec, (2 << 0) | (3 << 2) | (0 << 4) | (1 << 6))};
    }

    // Binary arithmetic operators.
    Vec4f& operator+=(const Vec4f& rhs) {
        vec = _mm_add_ps(vec, rhs.vec);
        return *this;
    }

    Vec4f& operator-=(const Vec4f& rhs) {
        vec = _mm_sub_ps(vec, rhs.vec);
        return *this;
    }

    Vec4f& operator*=(const Vec4f& rhs) {
        vec = _mm_mul_ps(vec, rhs.vec);
        return *this;
    }

    Vec4f& operator/=(const Vec4f& rhs) {
        vec = _mm_div_ps(vec, rhs.vec);
        return *this;
    }

    Vec4f& operator&=(const Vec4f& rhs) {
        vec = _mm_and_ps(vec, rhs.vec);
        return *this;
    }

    Vec4f& operator|=(const Vec4f& rhs) {
        vec = _mm_or_ps(vec, rhs.vec);
        return *this;
    }

    Vec4f& operator^=(const Vec4f& rhs) {
        vec = _mm_xor_ps(vec, rhs.vec);
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
inline Vec4f operator+(Vec4f lhs, const Vec4f& rhs) {
    lhs += rhs;
    return lhs;
}

inline Vec4f operator-(Vec4f lhs, const Vec4f& rhs) {
    lhs -= rhs;
    return lhs;
}

inline Vec4f operator*(Vec4f lhs, const Vec4f& rhs) {
    lhs *= rhs;
    return lhs;
}

inline Vec4f operator/(Vec4f lhs, const Vec4f& rhs) {
    lhs /= rhs;
    return lhs;
}

inline Vec4f operator&(Vec4f lhs, const Vec4f& rhs) {
    lhs &= rhs;
    return lhs;
}

inline Vec4f operator|(Vec4f lhs, const Vec4f& rhs) {
    lhs |= rhs;
    return lhs;
}

inline Vec4f operator^(Vec4f lhs, const Vec4f& rhs) {
    lhs ^= rhs;
    return lhs;
}

// Comparison operators.
inline Vec4f operator==(const Vec4f& lhs, const Vec4f& rhs) {
    return _mm_cmpeq_ps(lhs.vec, rhs.vec);
}

inline Vec4f operator!=(const Vec4f& lhs, const Vec4f& rhs) {
    return _mm_cmpneq_ps(lhs.vec, rhs.vec);
}

inline Vec4f operator<(const Vec4f& lhs, const Vec4f& rhs) {
    return _mm_cmplt_ps(lhs.vec, rhs.vec);
}

inline Vec4f operator>(const Vec4f& lhs, const Vec4f& rhs) {
    return _mm_cmpgt_ps(lhs.vec, rhs.vec);
}

inline Vec4f operator<=(const Vec4f& lhs, const Vec4f& rhs) {
    return _mm_cmple_ps(lhs.vec, rhs.vec);
}

inline Vec4f operator>=(const Vec4f& lhs, const Vec4f& rhs) {
    return _mm_cmpge_ps(lhs.vec, rhs.vec);
}

} // End namespace Common
