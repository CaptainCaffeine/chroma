// This file is a part of Chroma.
// Copyright (C) 2017 Matthew Murray
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

#include "core/Audio.h"
#include "core/memory/Memory.h"

namespace Core {

void Audio::UpdateAudio() {
    // Increment the sample counter and reset it on every frame.
    sample_drop = (sample_drop + 1) % 35112;

    FrameSequencerTick();

    UpdatePowerOnState();
    if (!audio_on) {
        // Queue silence when audio is off.
        QueueSample(0x00, 0x00);
        return;
    }

    square1.CheckTrigger(mem->console);
    square2.CheckTrigger(mem->console);
    wave.CheckTrigger(mem->console);

    square1.SweepTick(frame_seq_counter);

    square1.TimerTick();
    square2.TimerTick();
    wave.TimerTick();

    square1.LengthCounterTick(frame_seq_counter);
    square2.LengthCounterTick(frame_seq_counter);
    wave.LengthCounterTick(frame_seq_counter);

    square1.EnvelopeTick(frame_seq_counter);
    square2.EnvelopeTick(frame_seq_counter);

    u8 left_sample = 0x00;
    u8 right_sample = 0x00;

    u8 sample_channel1 = square1.GenSample();
    u8 sample_channel2 = square2.GenSample();
    u8 sample_channel3 = wave.GenSample();

    if (square1.EnabledLeft(sound_select)) {
        left_sample += sample_channel1;
    }

    if (square2.EnabledLeft(sound_select)) {
        left_sample += sample_channel2;
    }

    if (wave.EnabledLeft(sound_select)) {
        left_sample += sample_channel3;
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

    QueueSample(left_sample, right_sample);
}

void Audio::FrameSequencerTick() {
    frame_seq_clock += 2;

    bool frame_seq_inc = frame_seq_clock & 0x2000;
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

            frame_seq_counter = 0x00;
        }
    }
}

void Audio::ClearRegisters() {
    square1.ClearRegisters(mem->console);
    square2.ClearRegisters(mem->console);
    wave.ClearRegisters(mem->console);
    noise.ClearRegisters(mem->console);

    master_volume = 0x00;
    sound_select = 0x00;
    sound_on = 0x00;
}

void Audio::QueueSample(u8 left_sample, u8 right_sample) {
    // Take every 44th sample to get 1596 samples per frame. We need 1600 samples per frame for 48kHz at 60FPS,
    // so take another sample at 1/4 and 3/4 through the frame.
    if (sample_drop % 44 == 0 || sample_drop == 8778 || sample_drop == 26334) {
        // Multiply the samples by the master volume. This is done after the DAC and after the channels have been
        // mixed, and so can be greater than 0x0F.
        // The largest value is (0xF * 4) * 7 = 420, which is larger than 255, so we need to divide the samples by 2.
        // Otherwise the samples will overflow. It's also just way too loud.
        left_sample = (left_sample * (((master_volume & 0x70) >> 4) + 1)) >> 1;
        right_sample = (right_sample * ((master_volume & 0x07) + 1)) >> 1;
        sample_buffer.push_back(left_sample);
        sample_buffer.push_back(right_sample);
    }
}

u8 Audio::ReadNR52() const {
    return sound_on | 0x70 | square1.EnabledFlag() | square2.EnabledFlag() | wave.EnabledFlag() | noise.EnabledFlag();
}

} // End namespace Core
