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
#include "gb/core/GameBoy.h"
#include "common/Biquad.h"

namespace Gb {

Audio::Audio(bool enable_filter, const GameBoy& _gameboy)
        : gameboy(_gameboy)
        , enable_iir(enable_filter)
        , resample_buffer((enable_filter) ? interpolated_buffer_size * 2 : 0) {

    square1.LinkToAudio(this);
    square2.LinkToAudio(this);
    wave.LinkToAudio(this);
    noise.LinkToAudio(this);

    for (unsigned int i = 0; i < q.size(); ++i) {
        left_biquads.emplace_back(interpolated_buffer_size, q[i]);
        right_biquads.emplace_back(interpolated_buffer_size, q[i]);
    }
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

    square1.CheckTrigger(gameboy.console);
    square2.CheckTrigger(gameboy.console);
    wave.CheckTrigger(gameboy.console);
    noise.CheckTrigger(gameboy.console);

    square1.SweepTick();

    square1.TimerTick();
    square2.TimerTick();
    wave.TimerTick();
    noise.TimerTick();

    square1.LengthCounterTick();
    square2.LengthCounterTick();
    wave.LengthCounterTick();
    noise.LengthCounterTick();

    square1.EnvelopeTick();
    square2.EnvelopeTick();
    noise.EnvelopeTick();

    u8 left_sample = 0x00;
    u8 right_sample = 0x00;

    u8 sample_channel1 = square1.GenSample();
    u8 sample_channel2 = square2.GenSample();
    u8 sample_channel3 = wave.GenSample();
    u8 sample_channel4 = noise.GenSample();

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
    square1.ClearRegisters(gameboy.console);
    square2.ClearRegisters(gameboy.console);
    wave.ClearRegisters(gameboy.console);
    noise.ClearRegisters(gameboy.console);

    master_volume = 0x00;
    sound_select = 0x00;
    sound_on = 0x00;
}

void Audio::QueueSample(u8 left_sample, u8 right_sample) {
    if (enable_iir) {
        sample_counter += 1;

        // We pre-downsample the signal by 7 so the IIR filter can run in real time. So technically aliasing can still
        // occur, but it's much better than nearest-neighbour downsampling by 44.
        if (sample_counter % divisor == 0) {
            // Multiply the samples by the master volume. This is done after the DAC and after the channels have been
            // mixed, and so the final sample value can be greater than 0x0F.
            sample_buffer.push_back(left_sample * (((master_volume & 0x70) >> 4) + 1));
            sample_buffer.push_back(right_sample * ((master_volume & 0x07) + 1));
        }

        if (sample_buffer.size() == num_samples * 2) {
            Resample();
            sample_buffer.clear();
        }
    } else {
        sample_counter += 1 % 35112;

        // Take every 44th sample to get 1596 samples per frame. We need 800 samples per channel per frame for 
        // 48kHz at 60FPS, so take another sample at 1/4 and 3/4 through the frame.
        if (sample_counter % 44 == 0 || sample_counter == 8778 || sample_counter == 26334) {
            // Multiply the samples by the master volume. This is done after the DAC and after the channels have been
            // mixed, and so can be greater than 0x0F.
            sample_buffer.push_back(left_sample * (((master_volume & 0x70) >> 4) + 1));
            sample_buffer.push_back(right_sample * ((master_volume & 0x07) + 1));
        }

        if (sample_buffer.size() == output_buffer.size()) {
            for (std::size_t i = 0; i < sample_buffer.size(); ++i) {
                output_buffer[i] = sample_buffer[i] * 64;
            }

            sample_buffer.clear();
        }
    }
}

u8 Audio::ReadNR52() const {
    return sound_on | 0x70 | square1.EnabledFlag() | square2.EnabledFlag() | wave.EnabledFlag() | noise.EnabledFlag();
}

void Audio::Resample() {
    // The Game Boy generates 35112 samples per channel per frame, which we pre-downsample to 5016. We then resample
    // to 800 samples per channel by interpolating by a factor of 100 and decimating by a factor of 627.
    std::fill(resample_buffer.begin(), resample_buffer.end(), 0.0);

    for (std::size_t i = 0; i < num_samples; ++i) {
        // Multiply the samples by 64 to increase the amplitude for the IIR filter.
        resample_buffer[i * interpolation_factor * 2] = sample_buffer[i * 2] * 64.0;
        resample_buffer[i * interpolation_factor * 2 + 1] = sample_buffer[i * 2 + 1] * 64.0;
    }

    Common::Biquad::LowPassFilter(resample_buffer, left_biquads, right_biquads);

    for (std::size_t i = 0; i < output_buffer.size() / 2; ++i) {
        // Multiply by 64 to scale the volume for s16 samples.
        output_buffer[i * 2] = resample_buffer[i * decimation_factor * 2] * 64;
        output_buffer[i * 2 + 1] = resample_buffer[i * decimation_factor * 2 + 1] * 64;
    }
}

} // End namespace Gb
