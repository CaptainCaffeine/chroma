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

#include "common/Vec4f.h"

namespace Common {

class Biquad {
public:
    Biquad() = default;
    Biquad(int interpolated_buffer_size, float q1, float q2) {
        const float sampling_frequency = interpolated_buffer_size * 60.0f;
        const float k = std::tan(M_PI * cutoff_frequency / sampling_frequency);
        const float norm1 = 1.0f / (1.0f + k / q1 + k * k);
        const float norm2 = 1.0f / (1.0f + k / q2 + k * k);

        const Vec4f q_vec{q1, q1, q2, q2};
        const Vec4f k_vec{k, k, k, k};
        const Vec4f norm_vec{norm1, norm1, norm2, norm2};
        constexpr Vec4f two_vec{2, 2, 2, 2};
        constexpr Vec4f one_vec{1, 1, 1, 1};

        // The coefficients are for a two-pass Butterworth lowpass IIR filter. The first biquad is packed in the
        // low half of the Vec4f coefficients, and the second biquad in the high half.
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

    Vec4f FilterLowSample(Vec4f input) {
        // Make a copy of biquad2's Z values.
        const Vec4f high_z1 = z1;
        const Vec4f high_z2 = z2;

        // Filter the low/even sample through biquad1, and copy the unfiltered high/odd sample.
        const Vec4f high_sample = input;
        const Vec4f output = Vec4f::Combine(Filter(input), high_sample);

        // Restore biquad2's original Z values, since we just messed them up by filtering the high sample
        // through biquad2.
        z1 = Vec4f::Combine(z1, high_z1);
        z2 = Vec4f::Combine(z2, high_z2);

        return output;
    }

    Vec4f FilterHighSample(Vec4f input) {
        // Make a copy of biquad1's Z values.
        const Vec4f low_z1 = z1;
        const Vec4f low_z2 = z2;

        // Filter the high/odd sample through biquad2, and copy the low/even sample.
        const Vec4f low_sample = input;
        const Vec4f output = Vec4f::Combine(low_sample, Filter(input));

        // Restore biquad1's original Z values, since we just messed them up by filtering the low sample
        // through biquad1.
        z1 = Vec4f::Combine(low_z1, z1);
        z2 = Vec4f::Combine(low_z2, z2);

        return output;
    }

    static void LowPassFilter(std::vector<Vec4f>& resample_buffer, Biquad& biquad) {
        // Even numbered stereo samples are packed in the low half of the Vec4f, and odd numbered samples in the
        // high half. We process the left and right channels in parallel for each stereo sample, and we process
        // stereo sample i and i+1 in parallel using SIMD.

        // Filter sample 0 through biquad1 by itself.
        Vec4f filtering_samples = biquad.FilterLowSample(resample_buffer[0]);

        // Now swap the unfiltered sample 1 into the low half of a Vec4f, and the filtered sample 0 into the high half.
        filtering_samples = Vec4f::Swap(filtering_samples);

        for (unsigned int i = 1; i < resample_buffer.size(); ++i) {
            // Filter the odd sample through biquad1 and the even sample through biquad2.
            filtering_samples = biquad.Filter(filtering_samples);

            // Restore the original sample order.
            filtering_samples = Vec4f::Swap(filtering_samples);
            // Make a copy of the twice-filtered even sample.
            Vec4f finished_samples{filtering_samples};

            const Vec4f next_samples = resample_buffer[i];

            // Copy the next even sample to the low half of filtering_samples.
            filtering_samples = Vec4f::Combine(next_samples, filtering_samples);
            // Filter the next even sample through biquad1 and the previous odd sample through biquad2.
            filtering_samples = biquad.Filter(filtering_samples);

            // Reunite the twice-filtered odd sample with the previous even sample.
            finished_samples = Vec4f::Combine(finished_samples, filtering_samples);
            resample_buffer[i - 1] = finished_samples;

            // Copy the next odd sample into the low half of filtering_samples and the filtered even sample
            // into the high half.
            filtering_samples = Vec4f::CombineAndSwap(filtering_samples, next_samples);
        }

        // Filter the last odd sample through biquad1 and the last even sample through biquad2.
        filtering_samples = biquad.Filter(filtering_samples);

        // Restore the original sample order.
        filtering_samples = Vec4f::Swap(filtering_samples);

        // Run the last odd sample through biquad2 by itself.
        filtering_samples = biquad.FilterHighSample(filtering_samples);
        resample_buffer[resample_buffer.size() - 1] = filtering_samples;
    }

private:
    static constexpr float cutoff_frequency = 24000.0f;

    Vec4f a0;
    Vec4f a1;
    // a2 == a0;
    Vec4f b1;
    Vec4f b2;

    Vec4f z1 = {0.0f, 0.0f, 0.0f, 0.0f};
    Vec4f z2 = {0.0f, 0.0f, 0.0f, 0.0f};
};

} // End namespace Common
