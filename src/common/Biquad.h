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

#include <vector>
#include <cmath>
#include <emmintrin.h>

#include "common/Vec2d.h"
#include "common/Vec4f.h"

namespace Common {

class Biquad {
public:
    Biquad() = default;
    Biquad(int interpolated_buffer_size, float q) {
        const float sampling_frequency = interpolated_buffer_size * 60.0f;
        const float k = std::tan(M_PI * cutoff_frequency / sampling_frequency);
        const float norm = 1.0f / (1.0f + k / q + k * k);

        const Vec4f q_vec{q, q};
        const Vec4f k_vec{k, k};
        const Vec4f norm_vec{norm, norm};
        constexpr Vec4f two_vec{2, 2};
        constexpr Vec4f one_vec{1, 1};

        a0 = k_vec * k_vec * norm_vec;
        a1 = two_vec * a0;
        b1 = two_vec * (k_vec * k_vec - one_vec) * norm_vec;
        b2 = (one_vec - k_vec / q_vec + k_vec * k_vec) * norm_vec;
    }

    Vec4f Filter(Vec4f input) {
        // Biquad form used is the Transposed Direct Form 2.
        const Vec4f in_a0 = input * a0;
        const Vec4f output = z2 + in_a0;
        z2 = input * a1 - output * b1 + z1;
        z1 = in_a0 - output * b2;

        return output;
    }

    static void LowPassFilter(std::vector<Vec4f>& resample_buffer, std::vector<Biquad>& biquads) {
        // Butterworth lowpass IIR filter.
        for (unsigned int i = 0; i < resample_buffer.size(); ++i) {
            Vec4f sample_lo = resample_buffer[i];
            Vec4f sample_hi{_mm_movehl_ps(sample_lo.vec, sample_lo.vec)};
            for (auto& biquad : biquads) {
                sample_lo = biquad.Filter(sample_lo);
                sample_hi = biquad.Filter(sample_hi);
            }

            Vec4f filtered_sample{_mm_movelh_ps(sample_lo.vec, sample_hi.vec)};
            resample_buffer[i] = filtered_sample;
        }
    }

private:
    static constexpr float cutoff_frequency = 24000.0f;

    Vec4f a0;
    Vec4f a1;
    // a2 == a0;
    Vec4f b1;
    Vec4f b2;

    Vec4f z1 = {0.0f, 0.0f};
    Vec4f z2 = {0.0f, 0.0f};
};

} // End namespace Common
