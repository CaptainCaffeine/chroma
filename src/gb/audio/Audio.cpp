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

namespace Gb {

Audio::Audio(bool enable_filter, const Console _console)
        : square1(Generator::Square1, wave_ram, 0x00, 0x80, 0xF3, 0xFF, 0x00, _console)
        , square2(Generator::Square2, wave_ram, 0x00, 0x00, 0x00, 0xFF, 0x00, _console)
        , wave(Generator::Wave, wave_ram,       0x00, 0x00, 0x00, 0xFF, 0x00, _console)
        , noise(Generator::Noise, wave_ram,     0x00, 0x00, 0x00, 0x00, 0x00, _console)
        , enable_iir(enable_filter)
        , resample_buffer(enable_iir ? (interpolated_buffer_size / 2) : 0) {

    Common::Vec4f::SetFlushToZero();
}

// Needed to declare std::vector with forward-declared type in the header file.
Audio::~Audio() = default;

void Audio::UpdateAudio() {
    audio_clock += 2;

    UpdatePowerOnState();
    if (!audio_on) {
        // Queue silence when audio is off.
        QueueSample(0x00, 0x00);
        return;
    }

    const u32 frame_seq = GetFrameSequencer();

    square1.CheckTrigger(frame_seq);
    square2.CheckTrigger(frame_seq);
    wave.CheckTrigger(frame_seq);
    noise.CheckTrigger(frame_seq);

    square1.SweepTick(frame_seq);

    square1.TimerTick();
    square2.TimerTick();
    wave.TimerTick();
    noise.TimerTick();

    square1.LengthCounterTick(frame_seq);
    square2.LengthCounterTick(frame_seq);
    wave.LengthCounterTick(frame_seq);
    noise.LengthCounterTick(frame_seq);

    square1.EnvelopeTick(frame_seq);
    square2.EnvelopeTick(frame_seq);
    noise.EnvelopeTick(frame_seq);

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
        resample_buffer[sample_counter * interpolation_factor / 2] = Common::Vec4f{left_sample, right_sample};
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
    Common::Biquad::LowPassFilter(resample_buffer, biquad);

    for (std::size_t i = 0; i < output_buffer.size() / 2; ++i) {
        const bool index_is_even = (i * decimation_factor) % 2 == 0;
        auto [left_sample, right_sample] = resample_buffer[i * decimation_factor / 2].UnpackSamples(index_is_even);

        output_buffer[i * 2] = left_sample * 8;
        output_buffer[i * 2 + 1] = right_sample * 8;
    }

    std::fill(resample_buffer.begin(), resample_buffer.end(), Common::Vec4f{0.0f, 0.0f});
}

} // End namespace Gb
