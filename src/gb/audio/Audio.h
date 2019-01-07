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

#pragma once

#include <array>
#include <vector>
#include <numeric>

#include "common/CommonTypes.h"
#include "common/Vec4f.h"
#include "common/Biquad.h"
#include "gb/core/Enums.h"
#include "gb/audio/Channel.h"

namespace Gb {

class GameBoy;

class Audio {
public:
    Audio(bool enable_filter, const GameBoy& _gameboy);
    ~Audio();

    std::array<s16, 1600> output_buffer;

    Channel<Gen::Square1> square1;
    Channel<Gen::Square2> square2;
    Channel<Gen::Wave> wave;
    Channel<Gen::Noise> noise;

    u8 master_volume = 0x77;
    u8 sound_select = 0xF3;
    u8 sound_on = 0x80;

    std::array<u8, 0x20> wave_ram{{0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
                                   0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF}};

    void UpdateAudio();

    u8 ReadSoundOn() const;
    void WriteSoundRegs(const u16 addr, const u8 data);

private:
    const GameBoy& gameboy;

    u32 audio_clock = 0;

    // IIR filter
    static constexpr int samples_per_frame = 34960;
    static constexpr int interpolated_buffer_size = std::lcm(800, samples_per_frame);
    static constexpr int interpolation_factor = interpolated_buffer_size / samples_per_frame;
    static constexpr int decimation_factor = interpolated_buffer_size / 800;
    const bool enable_iir;
    int sample_counter = 0;

    std::vector<s16> sample_buffer;
    std::vector<Common::Vec4f> resample_buffer;

    // Q values are for a 4th order cascaded Butterworth lowpass filter.
    // Obtained from http://www.earlevel.com/main/2016/09/29/cascading-filters/.
    static constexpr std::array<float, 2> q{0.54119610f, 1.3065630f};
    Common::Biquad biquad{interpolated_buffer_size, q[0], q[1]};

    u32 GetFrameSequencer() const { return audio_clock >> 13; }

    void QueueSample(int left_sample, int right_sample);
    void Resample();

    void WriteSoundOn(u8 data);

    void ClearRegisters();

    int MasterVolumeRight() const { return master_volume & 0x7; }
    int MasterVolumeLeft() const { return (master_volume >> 4) & 0x7; }

    bool AudioEnabled() const { return sound_on & 0x80; }
};

} // End namespace Gb
