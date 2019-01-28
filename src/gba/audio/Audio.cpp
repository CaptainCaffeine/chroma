// This file is a part of Chroma.
// Copyright (C) 2018-2019 Matthew Murray
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

#include <algorithm>

#include "gba/audio/Audio.h"
#include "gba/core/Core.h"
#include "gba/hardware/Dma.h"
#include "gba/memory/Memory.h"

namespace Gba {

Audio::Audio(Core& _core)
        : square1(Gb::Console::AGB, true, 0x00, 0x00, 0x00, 0x00, 0x00)
        , square2(Gb::Console::AGB, true, 0x00, 0x00, 0x00, 0x00, 0x00)
        , wave(Gb::Console::AGB, true, 0x00, 0x00, 0x00, 0x00, 0x00)
        , noise(Gb::Console::AGB, true, 0x00, 0x00, 0x00, 0x00, 0x00)
        , core(_core)
        , resample_buffer(interpolated_buffer_size / 2) {

    Common::Vec4f::SetFlushToZero();
}

// Needed to declare std::vector with forward-declared type in the header file.
Audio::~Audio() = default;

void Audio::Update(int cycles) {
    const u64 updated_clock = audio_clock + cycles;

    if (!AudioEnabled()) {
        // Queue silence while audio is disabled.
        sample_count += updated_clock / 8 - audio_clock / 8;
        if (sample_count >= samples_per_frame) {
            Resample();
            sample_count %= samples_per_frame;
        }

        audio_clock = updated_clock;
        return;
    }

    // The APU runs at 2MHz, so it only updates every 8 cycles.
    while (audio_clock / 8 < updated_clock / 8) {
        audio_clock += 8;

        int left_sample = 0;
        int right_sample = 0;

        for (int f = 0; f < 2; ++f) {
            const int fifo_sample = (fifos[f].ReadCurrentSample(audio_clock) << 2) >> FifoVolume(f);

            if (FifoEnabledLeft(f)) {
                left_sample += fifo_sample;
            }

            if (FifoEnabledRight(f)) {
                right_sample += fifo_sample;
            }
        }

        square1.Update(GetFrameSequencer(), wave_ram);
        square2.Update(GetFrameSequencer(), wave_ram);
        wave.Update(GetFrameSequencer(), wave_ram);
        noise.Update(GetFrameSequencer(), wave_ram);

        const int sample_square1 = square1.GenSample();
        const int sample_square2 = square2.GenSample();
        const int sample_wave = wave.GenSample();
        const int sample_noise = noise.GenSample();

        int left_psg_sample = 0;
        int right_psg_sample = 0;

        const u8 enabled_channels = PsgEnabledChannels();

        if (square1.EnabledLeft(enabled_channels))  { left_psg_sample += sample_square1; }
        if (square1.EnabledRight(enabled_channels)) { right_psg_sample += sample_square1; }
        if (square2.EnabledLeft(enabled_channels))  { left_psg_sample += sample_square2; }
        if (square2.EnabledRight(enabled_channels)) { right_psg_sample += sample_square2; }
        if (wave.EnabledLeft(enabled_channels))     { left_psg_sample += sample_wave; }
        if (wave.EnabledRight(enabled_channels))    { right_psg_sample += sample_wave; }
        if (noise.EnabledLeft(enabled_channels))    { left_psg_sample += sample_noise; }
        if (noise.EnabledRight(enabled_channels))   { right_psg_sample += sample_noise; }

        left_psg_sample *= PsgVolumeLeft() + 1;
        right_psg_sample *= PsgVolumeRight() + 1;

        left_sample += left_psg_sample >> PsgMixerVolume();
        right_sample += right_psg_sample >> PsgMixerVolume();

        left_sample = ClampSample(left_sample);
        right_sample = ClampSample(right_sample);

        resample_buffer[sample_count * interpolation_factor / 2] = Common::Vec4f{left_sample, right_sample};
        sample_count += 1;

        if (sample_count == samples_per_frame) {
            Resample();
            sample_count = 0;
        }
    }

    audio_clock = updated_clock;
}

void Audio::Resample() {
    Common::Biquad::LowPassFilter(resample_buffer, biquad);

    for (int i = 0; i < 800; ++i) {
        const bool index_is_even = (i * decimation_factor) % 2 == 0;
        auto [left_sample, right_sample] = resample_buffer[i * decimation_factor / 2].UnpackSamples(index_is_even);

        output_buffer[i * 2] = left_sample * 4;
        output_buffer[i * 2 + 1] = right_sample * 4;
    }

    core.PushBackAudio(output_buffer);
    std::fill(resample_buffer.begin(), resample_buffer.end(), Common::Vec4f{0.0f, 0.0f});
}

int Audio::NextEvent() {
    const int remaining_samples = samples_per_frame - sample_count;
    int next_event_cycles = remaining_samples * 8 - audio_clock % 8;

    for (int f = 0; f < 2; ++f) {
        const int fifo_timer = FifoTimerSelect(f);
        const int next_timer_event_cycles =
            core.next_timer_event_cycles[fifo_timer] - core.timer_cycle_counter[fifo_timer];

        if (next_timer_event_cycles != 0) {
            next_event_cycles = std::min(next_event_cycles, next_timer_event_cycles);
        }

        if (fifo_timer == FifoTimerSelect(1)) {
            // No need to check both fifos if they're using the same timer.
            break;
        }
    }

    return next_event_cycles;
}

u16 Audio::ReadSoundOn() {
    Update(core.audio_cycle_counter);
    core.audio_cycle_counter = 0;
    core.next_audio_event_cycles = NextEvent();

    return sound_on | square1.EnabledFlag() | square2.EnabledFlag() | wave.EnabledFlag() | noise.EnabledFlag();
}

void Audio::WriteSoundOn(u16 data, u16 mask) {
    const bool was_enabled = AudioEnabled();

    sound_on.Write(data, mask);

    if (was_enabled && !AudioEnabled()) {
        ClearRegisters();
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

void Audio::ConsumeSample(int f, u64 timer_clock) {
    if (!AudioEnabled()) {
        return;
    }

    fifos[f].PopSample(timer_clock);

    if (fifos[f].NeedsMoreSamples()) {
        for (int i = 1; i < 3; ++i) {
            if (core.dma[i].WritingToFifo(f)) {
                core.dma[i].Trigger(Dma::Timing::Special);
                break;
            }
        }
    }
}

int Audio::ClampSample(int sample) const {
    // The bias is added to the final 10-bit sample. With the default bias of 0x200, this constrains the
    // output range to a signed 9-bit value (-0x200...0x1FF).
    sample += BiasLevel();
    sample = std::clamp(sample, 0, 0x3FF);
    sample -= BiasLevel();

    // We multiply the final sample by 64 to fill the s16 range.
    return sample * 64;
}

void Audio::ClearRegisters() {
    square1.ClearRegisters();
    square2.ClearRegisters();
    wave.ClearRegisters();
    noise.ClearRegisters();

    psg_control = 0x00;
}

s32 Fifo::ReadCurrentSample(u64 audio_clock) {
    // We maintain a queue of samples popped by the timer (play_queue) so the audio doesn't play any samples
    // too early. Once the emulated time in the audio hardware has surpassed the time a sample was queued,
    // we start playing that sample.
    if (play_queue.Size() > 0 && audio_clock >= play_queue.Read().second) {
        playing_sample = play_queue.PopFront().first;
    }

    return playing_sample;
}

void Fifo::PopSample(u64 timer_clock) {
    if (fifo_buffer.Size() == 0) {
        // Play silence if the fifo is empty.
        play_queue.PushBack({0, timer_clock});
        return;
    }

    // Pop the next sample from the fifo and add it to the play queue. We record the time the sample was popped so
    // we know when to start playing it.
    const s8 sample = fifo_buffer.PopFront();
    play_queue.PushBack({sample, timer_clock});
}

void Fifo::Write(u16 data, u16 mask_8bit) {
    if (fifo_buffer.Size() == fifo_length) {
        // The fifo is full.
        return;
    }

    if (mask_8bit == 0xFFFF) {
        // 16-bit write.
        fifo_buffer.PushBack(data & 0xFF);
        if (fifo_buffer.Size() != fifo_length) {
            fifo_buffer.PushBack(data >> 8);
        }
    } else {
        // 8-bit write.
        if (mask_8bit == 0x00FF) {
            fifo_buffer.PushBack(data & 0xFF);
        } else {
            fifo_buffer.PushBack(data >> 8);
        }
    }
}

void Fifo::Reset() {
    fifo_buffer.Reset();
    play_queue.Reset();
    playing_sample = 0;
}

void Audio::WriteSoundRegs(const u32 addr, const u16 data, const u16 mask) {
    Update(core.audio_cycle_counter);

    const bool write_low_byte = (mask & 0x00FF) == 0x00FF;
    const bool write_high_byte = (mask & 0xFF00) == 0xFF00;

    if (!AudioEnabled()) {
        // Only SOUNDCNT_H, SOUNDCNT_X, SOUNDBIAS, and wave RAM are accessible when audio is disabled.
        switch (addr & ~0x1) {
        case SOUNDCNT_H:
            WriteFifoControl(data, mask);
            break;
        case SOUNDCNT_X:
            WriteSoundOn(data, mask);
            break;
        case SOUNDBIAS:
            soundbias.Write(data, mask);
            break;
        case WAVE_RAM0_L:
        case WAVE_RAM0_H:
        case WAVE_RAM1_L:
        case WAVE_RAM1_H:
        case WAVE_RAM2_L:
        case WAVE_RAM2_H:
        case WAVE_RAM3_L:
        case WAVE_RAM3_H: {
            const u32 wave_ram_addr = addr - WAVE_RAM0_L + wave.AccessibleBankOffset();
            if (write_low_byte) {
                wave_ram[wave_ram_addr] = data;
            }
            if (write_high_byte) {
                wave_ram[wave_ram_addr + 1] = data >> 8;
            }
            break;
        }
        default:
            break;
        }

        core.audio_cycle_counter = 0;
        core.next_audio_event_cycles = NextEvent();

        return;
    }

    switch (addr & ~0x1) {
    case SOUND1CNT_L:
        if (write_low_byte) {
            square1.WriteSweep(data);
        }
        break;
    case SOUND1CNT_H:
        if (write_low_byte) {
            square1.WriteSoundLength(data);
        }
        if (write_high_byte) {
            square1.WriteEnvelope(data >> 8);
        }
        break;
    case SOUND1CNT_X:
        if (write_low_byte) {
            square1.WriteFrequencyLow(data);
        }
        if (write_high_byte) {
            square1.WriteReset(data >> 8, GetFrameSequencer());
        }
        break;
    case SOUND2CNT_L:
        if (write_low_byte) {
            square2.WriteSoundLength(data);
        }
        if (write_high_byte) {
            square2.WriteEnvelope(data >> 8);
        }
        break;
    case SOUND2CNT_H:
        if (write_low_byte) {
            square2.WriteFrequencyLow(data);
        }
        if (write_high_byte) {
            square2.WriteReset(data >> 8, GetFrameSequencer());
        }
        break;
    case SOUND3CNT_L:
        if (write_low_byte) {
            wave.WriteWaveControl(data);
        }
        break;
    case SOUND3CNT_H:
        if (write_low_byte) {
            wave.WriteSoundLength(data);
        }
        if (write_high_byte) {
            wave.WriteEnvelope(data >> 8);
        }
        break;
    case SOUND3CNT_X:
        if (write_low_byte) {
            wave.WriteFrequencyLow(data);
        }
        if (write_high_byte) {
            wave.WriteReset(data >> 8, GetFrameSequencer());
        }
        break;
    case SOUND4CNT_L:
        if (write_low_byte) {
            noise.WriteSoundLength(data);
        }
        if (write_high_byte) {
            noise.WriteEnvelope(data >> 8);
        }
        break;
    case SOUND4CNT_H:
        if (write_low_byte) {
            noise.WriteFrequencyLow(data);
        }
        if (write_high_byte) {
            noise.WriteReset(data >> 8, GetFrameSequencer());
        }
        break;
    case SOUNDCNT_L:
        psg_control.Write(data, mask);
        break;
    case SOUNDCNT_H:
        WriteFifoControl(data, mask);
        break;
    case SOUNDCNT_X:
        WriteSoundOn(data, mask);
        break;
    case SOUNDBIAS:
        soundbias.Write(data, mask);
        break;
    case WAVE_RAM0_L:
    case WAVE_RAM0_H:
    case WAVE_RAM1_L:
    case WAVE_RAM1_H:
    case WAVE_RAM2_L:
    case WAVE_RAM2_H:
    case WAVE_RAM3_L:
    case WAVE_RAM3_H: {
        const u32 wave_ram_addr = addr - WAVE_RAM0_L + wave.AccessibleBankOffset();
        if (write_low_byte) {
            wave_ram[wave_ram_addr] = data;
        }
        if (write_high_byte) {
            wave_ram[wave_ram_addr + 1] = data >> 8;
        }
        break;
    }
    default:
        break;
    }

    core.audio_cycle_counter = 0;
    core.next_audio_event_cycles = NextEvent();
}

} // End namespace Gba
