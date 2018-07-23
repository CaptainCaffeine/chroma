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
#include <limits>

namespace Common {

class Biquad {
public:
    Biquad() = default;
    Biquad(int interpolated_buffer_size, double q)
            : sampling_frequency(interpolated_buffer_size * 60.0)
            , k(std::tan(M_PI * cutoff_frequency / sampling_frequency))
            , norm(1 / (1 + k / q + k * k))
            , a0(k * k * norm)
            , a1(2 * a0)
            , b1(2 * (k * k - 1) * norm)
            , b2((1 - k / q + k * k) * norm) {}

    double Filter(double input) {
        // Biquad form used is the Transposed Direct Form 2.
        const double in_a0 = input * a0;
        const double output = z2 + in_a0;
        z2 = input * a1 - output * b1 + z1;
        z1 = in_a0 - output * b2;

        return output;
    }

    static void LowPassFilter(std::vector<double>& resample_buffer,
                              std::vector<Biquad>& left_biquads,
                              std::vector<Biquad>& right_biquads) {
        // Butterworth lowpass IIR filter.
        for (unsigned int i = 0; i < resample_buffer.size() / 2; ++i) {
            // Left signal.
            double sample = resample_buffer[i * 2];
            for (auto& biquad : left_biquads) {
                sample = biquad.Filter(sample);
            }
            resample_buffer[i * 2] = sample;

            // Right signal.
            sample = resample_buffer[i * 2 + 1];
            for (auto& biquad : right_biquads) {
                sample = biquad.Filter(sample);
            }
            resample_buffer[i * 2 + 1] = sample;

            // This algorithm often generates denormal values in the z delays. These values are far too small
            // to impact the signal and they slow down the algorithm significantly, so we round denormal values to 0.
            constexpr double limit = std::numeric_limits<double>::min();
            for (unsigned int j = 0; j < left_biquads.size(); ++j) {
                if (std::abs(left_biquads[j].z1) < limit) { left_biquads[j].z1 = 0.0; }
                if (std::abs(left_biquads[j].z2) < limit) { left_biquads[j].z2 = 0.0; }
                if (std::abs(right_biquads[j].z1) < limit) { right_biquads[j].z1 = 0.0; }
                if (std::abs(right_biquads[j].z2) < limit) { right_biquads[j].z2 = 0.0; }
            }
        }
    }

private:
    static constexpr double cutoff_frequency = 24000.0;
    double sampling_frequency;

    double k;
    double norm;

    double a0;
    double a1;
    // a2 == a0;
    double b1;
    double b2;

    double z1 = 0.0;
    double z2 = 0.0;
};

} // End namespace Common
