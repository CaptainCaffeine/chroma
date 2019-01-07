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

#include <array>
#include <vector>
#include <numeric>

#include "common/CommonTypes.h"
#include "common/Vec4f.h"
#include "common/RingBuffer.h"
#include "common/Biquad.h"
#include "gba/memory/IOReg.h"
#include "gb/audio/Channel.h"

namespace Gba {

class Core;

class Fifo {
public:
    s32 ReadCurrentSample(u64 audio_clock);
    void PopSample(u64 timer_clock);
    void Write(u16 data, u16 mask_8bit);
    void Reset();
    bool NeedsMoreSamples() const { return fifo_buffer.Size() <= 16; }

private:
    static constexpr int fifo_length = 32;
    Common::RingBuffer<s8, fifo_length> fifo_buffer;

    // The largest number of samples in the play queue never went higher than 2 in my testing, but I just
    // went with a size of 4 to be safe.
    Common::RingBuffer<std::pair<s8, u64>, 4> play_queue;
    s8 playing_sample = 0;
};

class Audio {
public:
    explicit Audio(Core& _core);
    ~Audio();

    IOReg psg_control  = {0x0000, 0xFF77, 0xFF77};
    IOReg fifo_control = {0x0000, 0x770F, 0xFF0F};
    IOReg sound_on     = {0x0000, 0x008F, 0x0080};
    IOReg soundbias    = {0x0000, 0xC3FE, 0xC3FE};

    Gb::Channel<Gb::Gen::Square1> square1;
    Gb::Channel<Gb::Gen::Square2> square2;
    Gb::Channel<Gb::Gen::Wave> wave;
    Gb::Channel<Gb::Gen::Noise> noise;

    std::array<u8, 0x20> wave_ram{};

    std::array<Fifo, 2> fifos;
    std::array<s16, 1600> output_buffer;

    void Update(int cycles);
    void ConsumeSample(int f, u64 timer_clock);
    int NextEvent();

    void WriteSoundRegs(const u32 addr, const u16 data, const u16 mask);

    u16 ReadSoundOn();

    void WriteFifoControl(u16 data, u16 mask);
    int FifoTimerSelect(int f) const { return (fifo_control >> (10 + 4 * f)) & 0x1; }

private:
    Core& core;

    int sample_count = 0;
    u64 audio_clock = 0;

    static constexpr int samples_per_frame = 34960;
    static constexpr int interpolated_buffer_size = std::lcm(800, samples_per_frame);
    static constexpr int interpolation_factor = interpolated_buffer_size / samples_per_frame;
    static constexpr int decimation_factor = interpolated_buffer_size / 800;
    std::vector<Common::Vec4f> resample_buffer;

    // Q values are for a 4th order cascaded Butterworth lowpass filter.
    // Obtained from http://www.earlevel.com/main/2016/09/29/cascading-filters/.
    static constexpr std::array<float, 2> q{0.54119610f, 1.3065630f};
    Common::Biquad biquad{interpolated_buffer_size, q[0], q[1]};

    void Resample();
    int ClampSample(int sample) const;

    u64 GetFrameSequencer() const { return audio_clock >> 15; }

    void WriteSoundOn(u16 data, u16 mask);

    void ClearRegisters();

    int PsgVolumeRight() const { return psg_control & 0x7; }
    int PsgVolumeLeft() const { return (psg_control >> 4) & 0x7; }
    u8 PsgEnabledChannels() const { return psg_control >> 8; }

    int PsgMixerVolume() const { return 2 - (fifo_control & 0x3); }
    int FifoVolume(int f) const { return (~fifo_control >> (2 + f)) & 0x1; }
    bool FifoEnabledRight(int f) const { return (fifo_control >> (8 + 4 * f)) & 0x1; }
    bool FifoEnabledLeft(int f) const { return (fifo_control >> (9 + 4 * f)) & 0x1; }
    bool FifoReset(int f) const { return (fifo_control >> (11 + 4 * f)) & 0x1; }
    void ClearReset(int f) { fifo_control &= ~(1 << (11 + 4 * f)); }

    bool AudioEnabled() const { return sound_on & 0x80; }

    int BiasLevel() const { return soundbias & 0x02FE; }
    int Resolution() const { return (soundbias >> 14) & 0x3; }
};

} // End namespace Gba
