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

#include <numeric>

#include "gba/audio/Audio.h"
#include "gba/core/Core.h"
#include "gba/hardware/Dma.h"
#include "common/Biquad.h"

namespace Gba {

Audio::Audio(Core& _core)
        : core(_core) {

    Common::Vec2d::SetFlushToZero();
}

// Needed to declare std::vector with forward-declared type in the header file.
Audio::~Audio() = default;

void Audio::Update() {
    if (!AudioEnabled()) {
        return;
    }

    if (fifos[0].samples_per_frame <= 4 * 5 && fifos[1].samples_per_frame <= 4 * 5) {
        // Neither fifo has a timer running.
        return;
    }

    if (fifos[0].sample_buffer.size() == fifos[0].samples_per_frame
            || fifos[1].sample_buffer.size() == fifos[1].samples_per_frame) {

        // If only one fifo is in use, the sample rate of the inactive one will be 4 (times 5). If both fifos are in
        // use, we assume they're using the same sample rate.
        int samples_per_frame = std::max(fifos[0].samples_per_frame, fifos[1].samples_per_frame);

        std::fill(resample_buffer.begin(), resample_buffer.end(), Common::Vec2d{0.0, 0.0});

        if (samples_per_frame != prev_samples_per_frame) {
            // The sample rate changed, so we need to update the resampling parameters and biquads.
            interpolated_buffer_size = std::lcm(800, samples_per_frame);
            interpolation_factor = interpolated_buffer_size / samples_per_frame;
            decimation_factor = interpolated_buffer_size / 800;
            resample_buffer.resize(interpolated_buffer_size);

            biquads.clear();
            for (unsigned int i = 0; i < q.size(); ++i) {
                biquads.emplace_back(interpolated_buffer_size, q[i]);
            }

            prev_samples_per_frame = samples_per_frame;
        }

        for (int i = 0; i < samples_per_frame; ++i) {
            int left_sample = 0;
            int right_sample = 0;

            for (int f = 0; f < 2; ++f) {
                if (fifos[f].sample_buffer.size() == 0) {
                    continue;
                }

                const int fifo_sample = (static_cast<s32>(fifos[f].sample_buffer[i]) << 2) >> FifoVolume(f);

                if (FifoEnabledLeft(f)) {
                    left_sample += fifo_sample;
                }

                if (FifoEnabledRight(f)) {
                    right_sample += fifo_sample;
                }
            }

            left_sample = ClampSample(left_sample);
            right_sample = ClampSample(right_sample);

            resample_buffer[i * interpolation_factor] = Common::Vec2d{left_sample, right_sample};
        }

        fifos[0].sample_buffer.clear();
        fifos[1].sample_buffer.clear();

        Common::Biquad::LowPassFilter(resample_buffer, biquads);

        for (int i = 0; i < 800; ++i) {
            auto [left_sample, right_sample] = resample_buffer[i * decimation_factor].UnpackSamples();

            output_buffer[i * 2] = left_sample * 2;
            output_buffer[i * 2 + 1] = right_sample * 2;
        }
    }
}

void Audio::WriteFifoControl(u16 data, u16 mask) {
    fifo_control.Write(data, mask);

    for (int f = 0; f < 2; ++f) {
        if (FifoReset(f)) {
            fifos[f].Reset();
            ClearReset(f);
        }
    }
}

void Audio::ConsumeSample(int f) {
    if (!AudioEnabled()) {
        return;
    }

    fifos[f].ReadSample();

    if (fifos[f].size <= 16) {
        for (int i = 1; i < 3; ++i) {
            if (core.dma[i].WritingToFifo(f)) {
                core.dma[i].Trigger(Dma::Timing::Special);
                break;
            }
        }
    }
}

} // End namespace Gba
