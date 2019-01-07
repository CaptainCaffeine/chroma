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
#include "gb/memory/Memory.h"

namespace Gb {

Audio::Audio(bool enable_filter, const GameBoy& _gameboy)
        : square1(_gameboy.console, false, 0x00, 0x80, 0xF3, 0xFF, 0x00)
        , square2(_gameboy.console, false, 0x00, 0x00, 0x00, 0xFF, 0x00)
        , wave(_gameboy.console, false, 0x00, 0x00, 0x00, 0xFF, 0x00)
        , noise(_gameboy.console, false, 0x00, 0x00, 0x00, 0x00, 0x00)
        , gameboy(_gameboy)
        , enable_iir(enable_filter)
        , resample_buffer(enable_iir ? (interpolated_buffer_size / 2) : 0) {

    Common::Vec4f::SetFlushToZero();
}

// Needed to declare std::vector with forward-declared type in the header file.
Audio::~Audio() = default;

void Audio::UpdateAudio() {
    audio_clock += 2;

    if (!AudioEnabled()) {
        // Queue silence when audio is off.
        QueueSample(0x00, 0x00);
        return;
    }

    square1.Update(GetFrameSequencer(), wave_ram);
    square2.Update(GetFrameSequencer(), wave_ram);
    wave.Update(GetFrameSequencer(), wave_ram);
    noise.Update(GetFrameSequencer(), wave_ram);

    const int sample_channel1 = square1.GenSample();
    const int sample_channel2 = square2.GenSample();
    const int sample_channel3 = wave.GenSample();
    const int sample_channel4 = noise.GenSample();

    int left_sample = 0x00;
    int right_sample = 0x00;

    if (square1.EnabledLeft(sound_select))  { left_sample += sample_channel1; }
    if (square1.EnabledRight(sound_select)) { right_sample += sample_channel1; }
    if (square2.EnabledLeft(sound_select))  { left_sample += sample_channel2; }
    if (square2.EnabledRight(sound_select)) { right_sample += sample_channel2; }
    if (wave.EnabledLeft(sound_select))     { left_sample += sample_channel3; }
    if (wave.EnabledRight(sound_select))    { right_sample += sample_channel3; }
    if (noise.EnabledLeft(sound_select))    { left_sample += sample_channel4; }
    if (noise.EnabledRight(sound_select))   { right_sample += sample_channel4; }

    QueueSample(left_sample, right_sample);
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

u8 Audio::ReadSoundOn() const {
    return sound_on | 0x70 | square1.EnabledFlag() | square2.EnabledFlag() | wave.EnabledFlag() | noise.EnabledFlag();
}

void Audio::WriteSoundOn(u8 data) {
    const bool was_enabled = AudioEnabled();
    sound_on = data & 0x80;

    if (was_enabled && !AudioEnabled()) {
        ClearRegisters();
    }
}

void Audio::QueueSample(int left_sample, int right_sample) {
    // Multiply the samples by the master volume. This is done after the DAC and after the channels have been
    // mixed, and so the final sample value can be greater than 0x0F.
    left_sample *= MasterVolumeLeft() + 1;
    right_sample *= MasterVolumeRight() + 1;

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

void Audio::WriteSoundRegs(const u16 addr, const u8 data) {
    if (!AudioEnabled()) {
        // On DMG, the length counters are still read-writeable when audio is disabled.
        switch (addr) {
        case Memory::NR11:
            if (gameboy.ConsoleDmg()) {
                square1.WriteSoundLength(data);
            }
            break;
        case Memory::NR21:
            if (gameboy.ConsoleDmg()) {
                square2.WriteSoundLength(data);
            }
            break;
        case Memory::NR31:
            if (gameboy.ConsoleDmg()) {
                wave.WriteSoundLength(data);
            }
            break;
        case Memory::NR41:
            if (gameboy.ConsoleDmg()) {
                noise.WriteSoundLength(data);
            }
            break;
        case Memory::NR52:
            WriteSoundOn(data);
            break;
        case Memory::WAVE_0: case Memory::WAVE_1: case Memory::WAVE_2: case Memory::WAVE_3:
        case Memory::WAVE_4: case Memory::WAVE_5: case Memory::WAVE_6: case Memory::WAVE_7:
        case Memory::WAVE_8: case Memory::WAVE_9: case Memory::WAVE_A: case Memory::WAVE_B:
        case Memory::WAVE_C: case Memory::WAVE_D: case Memory::WAVE_E: case Memory::WAVE_F:
            wave_ram[addr - Memory::WAVE_0] = data;
            break;
        }

        return;
    }

    switch (addr) {
    case Memory::NR10:
        square1.WriteSweep(data);
        break;
    case Memory::NR11:
        square1.WriteSoundLength(data);
        break;
    case Memory::NR12:
        square1.WriteEnvelope(data);
        break;
    case Memory::NR13:
        square1.WriteFrequencyLow(data);
        break;
    case Memory::NR14:
        square1.WriteReset(data, GetFrameSequencer());
        break;
    case Memory::NR21:
        square2.WriteSoundLength(data);
        break;
    case Memory::NR22:
        square2.WriteEnvelope(data);
        break;
    case Memory::NR23:
        square2.WriteFrequencyLow(data);
        break;
    case Memory::NR24:
        square2.WriteReset(data, GetFrameSequencer());
        break;
    case Memory::NR30:
        wave.WriteWaveControl(data);
        break;
    case Memory::NR31:
        wave.WriteSoundLength(data);
        break;
    case Memory::NR32:
        wave.WriteEnvelope(data);
        break;
    case Memory::NR33:
        wave.WriteFrequencyLow(data);
        break;
    case Memory::NR34:
        wave.WriteReset(data, GetFrameSequencer());
        break;
    case Memory::NR41:
        noise.WriteSoundLength(data);
        break;
    case Memory::NR42:
        noise.WriteEnvelope(data);
        break;
    case Memory::NR43:
        noise.WriteFrequencyLow(data);
        break;
    case Memory::NR44:
        noise.WriteReset(data, GetFrameSequencer());
        break;
    case Memory::NR50:
        master_volume = data;
        break;
    case Memory::NR51:
        sound_select = data;
        break;
    case Memory::NR52:
        WriteSoundOn(data);
        break;
    case Memory::WAVE_0: case Memory::WAVE_1: case Memory::WAVE_2: case Memory::WAVE_3:
    case Memory::WAVE_4: case Memory::WAVE_5: case Memory::WAVE_6: case Memory::WAVE_7:
    case Memory::WAVE_8: case Memory::WAVE_9: case Memory::WAVE_A: case Memory::WAVE_B:
    case Memory::WAVE_C: case Memory::WAVE_D: case Memory::WAVE_E: case Memory::WAVE_F:
        wave_ram[addr - Memory::WAVE_0] = data;
        break;
    default:
        break;
    }
}

} // End namespace Gb
