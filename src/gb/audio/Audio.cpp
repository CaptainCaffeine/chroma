// This file is a part of Chroma.
// Copyright (C) 2017-2018 Matthew Murray
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

#include "gb/audio/Audio.h"
#include "common/Biquad.h"

namespace Gb {

Audio::Audio(bool enable_filter, const Console _console)
        : square1{Generator::Square1, wave_ram, 0x00, 0x80, 0xF3, 0xFF, 0x00, _console}
        , square2{Generator::Square2, wave_ram, 0x00, 0x00, 0x00, 0xFF, 0x00, _console}
        , wave{Generator::Wave, wave_ram,       0x00, 0x00, 0x00, 0xFF, 0x00, _console}
        , noise{Generator::Noise, wave_ram,     0x00, 0x00, 0x00, 0x00, 0x00, _console}
        , enable_iir(enable_filter)
        , resample_buffer(enable_iir ? interpolated_buffer_size : 0) {

    for (unsigned int i = 0; i < q.size(); ++i) {
        biquads.emplace_back(interpolated_buffer_size, q[i]);
    }

    Common::Vec2d::SetFlushToZero();
}

// Needed to declare std::vector with forward-declared type in the header file.
Audio::~Audio() = default;

void Audio::UpdateAudio() {
    FrameSequencerTick();

    UpdatePowerOnState();
    if (!audio_on) {
        // Queue silence when audio is off.
        QueueSample(0x00, 0x00);
        return;
    }

    square1.CheckTrigger(frame_seq_counter);
    square2.CheckTrigger(frame_seq_counter);
    wave.CheckTrigger(frame_seq_counter);
    noise.CheckTrigger(frame_seq_counter);

    square1.SweepTick(frame_seq_counter);

    square1.TimerTick();
    square2.TimerTick();
    wave.TimerTick();
    noise.TimerTick();

    square1.LengthCounterTick(frame_seq_counter);
    square2.LengthCounterTick(frame_seq_counter);
    wave.LengthCounterTick(frame_seq_counter);
    noise.LengthCounterTick(frame_seq_counter);

    square1.EnvelopeTick(frame_seq_counter);
    square2.EnvelopeTick(frame_seq_counter);
    noise.EnvelopeTick(frame_seq_counter);

    int left_sample = 0x00;
    int right_sample = 0x00;

    const int sample_channel1 = square1.GenSample();
    const int sample_channel2 = square2.GenSample();
    const int sample_channel3 = wave.GenSample();
    const int sample_channel4 = noise.GenSample();

    if (square1.EnabledLeft(sound_select)) {
        left_sample += sample_channel1;
    }

    if (square2.EnabledLeft(sound_select)) {
        left_sample += sample_channel2;
    }

    if (wave.EnabledLeft(sound_select)) {
        left_sample += sample_channel3;
    }

    if (noise.EnabledLeft(sound_select)) {
        left_sample += sample_channel4;
    }

    if (square1.EnabledRight(sound_select)) {
        right_sample += sample_channel1;
    }

    if (square2.EnabledRight(sound_select)) {
        right_sample += sample_channel2;
    }

    if (wave.EnabledRight(sound_select)) {
        right_sample += sample_channel3;
    }

    if (noise.EnabledRight(sound_select)) {
        right_sample += sample_channel4;
    }

    QueueSample(left_sample, right_sample);
}

void Audio::FrameSequencerTick() {
    frame_seq_clock += 2;

    bool frame_seq_inc = frame_seq_clock & 0x1000;
    if (!frame_seq_inc && prev_frame_seq_inc) {
        frame_seq_counter += 1;
    }

    prev_frame_seq_inc = frame_seq_inc;
}

void Audio::UpdatePowerOnState() {
    bool audio_power_on = sound_on & 0x80;
    if (audio_power_on != audio_on) {
        audio_on = audio_power_on;

        if (!audio_on) {
            ClearRegisters();
        } else {
            square1.PowerOn();
            square2.PowerOn();
            wave.PowerOn();

            frame_seq_counter = 0x00;
        }
    }
}

void Audio::ClearRegisters() {
    square1.ClearRegisters();
    square2.ClearRegisters();
    wave.ClearRegisters();
    noise.ClearRegisters();

    master_volume = 0x00;
    sound_select = 0x00;
    sound_on = 0x00;
}

u8 Audio::ReadNR52() const {
    return sound_on | 0x70 | square1.EnabledFlag() | square2.EnabledFlag() | wave.EnabledFlag() | noise.EnabledFlag();
}

void Audio::QueueSample(int left_sample, int right_sample) {
    // Multiply the samples by the master volume. This is done after the DAC and after the channels have been
    // mixed, and so the final sample value can be greater than 0x0F.
    left_sample *= ((master_volume & 0x70) >> 4) + 1;
    right_sample *= (master_volume & 0x07) + 1;

    // Multiply by 64 to scale the volume for s16 samples.
    left_sample *= 64;
    right_sample *= 64;

    if (enable_iir) {
        resample_buffer[sample_counter * interpolation_factor] = Common::Vec2d{left_sample, right_sample};
        sample_counter += 1;

        if (sample_counter == samples_per_frame) {
            Resample();
            sample_counter = 0;
        }
    } else {
        sample_counter += 1;

        // Take every 44th sample to get 795 samples per frame. We need 800 samples per channel per frame for
        // 48kHz at 60FPS, so we take five more throughout the frame.
        if (sample_counter % 44 == 0
                || sample_counter % (samples_per_frame / 5) == 0
                || sample_counter % (samples_per_frame / 2) == 0) {
            sample_buffer.push_back(left_sample);
            sample_buffer.push_back(right_sample);
        }

        if (sample_counter == samples_per_frame) {
            std::copy(sample_buffer.cbegin(), sample_buffer.cend(), output_buffer.begin());
            sample_buffer.clear();
            sample_counter = 0;
        }
    }
}

void Audio::Resample() {
    Common::Biquad::LowPassFilter(resample_buffer, biquads);

    for (std::size_t i = 0; i < output_buffer.size() / 2; ++i) {
        auto [left_sample, right_sample] = resample_buffer[i * decimation_factor].UnpackSamples();

        output_buffer[i * 2] = left_sample * 8;
        output_buffer[i * 2 + 1] = right_sample * 8;
    }

    std::fill(resample_buffer.begin(), resample_buffer.end(), Common::Vec2d{0.0, 0.0});
}

} // End namespace Gb
