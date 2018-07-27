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

namespace Common {

class Biquad {
public:
    Biquad() = default;
    Biquad(int interpolated_buffer_size, double q) {
        const double sampling_frequency = interpolated_buffer_size * 60.0;
        const double k = std::tan(M_PI * cutoff_frequency / sampling_frequency);
        const double norm = 1 / (1 + k / q + k * k);

        const Vec2d q_vec{q, q};
        const Vec2d k_vec{k, k};
        const Vec2d norm_vec{norm, norm};
        constexpr Vec2d two_vec{2, 2};
        constexpr Vec2d one_vec{1, 1};

        a0 = k_vec * k_vec * norm_vec;
        a1 = two_vec * a0;
        b1 = two_vec * (k_vec * k_vec - one_vec) * norm_vec;
        b2 = (one_vec - k_vec / q_vec + k_vec * k_vec) * norm_vec;
    }

    Vec2d Filter(Vec2d input) {
        // Biquad form used is the Transposed Direct Form 2.
        const Vec2d in_a0 = input * a0;
        const Vec2d output = z2 + in_a0;
        z2 = input * a1 - output * b1 + z1;
        z1 = in_a0 - output * b2;

        return output;
    }

    static void LowPassFilter(std::vector<Vec2d>& resample_buffer, std::vector<Biquad>& biquads) {
        // Butterworth lowpass IIR filter.
        for (unsigned int i = 0; i < resample_buffer.size(); ++i) {
            Vec2d sample = resample_buffer[i];
            for (auto& biquad : biquads) {
                sample = biquad.Filter(sample);
            }
            resample_buffer[i] = sample;
        }
    }

private:
    static constexpr double cutoff_frequency = 24000.0;

    Vec2d a0;
    Vec2d a1;
    // a2 == a0;
    Vec2d b1;
    Vec2d b2;

    Vec2d z1 = {0.0, 0.0};
    Vec2d z2 = {0.0, 0.0};
};

} // End namespace Common
