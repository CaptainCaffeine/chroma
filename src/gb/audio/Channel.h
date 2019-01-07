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

#include "common/CommonTypes.h"
#include "gb/core/Enums.h"

namespace Gb {

enum Gen : u8 {Square1 = 0x01,
               Square2 = 0x02,
               Wave    = 0x04,
               Noise   = 0x08};

template<Gen G>
class Channel {
public:
    Channel(const Console _console, bool _gba_mode, u8 NRx0, u8 NRx1, u8 NRx2, u8 NRx3, u8 NRx4);

    void Update(u32 frame_seq, const std::array<u8, 0x20>& wave_ram);

    u8 GenSample() const;

    u8 EnabledFlag() const { return (channel_enabled) ? right_enable_mask : 0x00; }
    bool EnabledLeft(const u8 sound_select) const { return channel_enabled && (sound_select & left_enable_mask); }
    bool EnabledRight(const u8 sound_select) const { return channel_enabled && (sound_select & right_enable_mask); }

    u16 ReadSweepGba() const { return sweep; }
    u16 ReadDutyAndEnvelopeGba() const { return sound_length | (volume_envelope << 8); }
    u16 ReadResetGba() const;

    u8 ReadSweepCgb() const;
    u8 ReadSoundLengthCgb() const { return sound_length | 0x3F; }
    u8 ReadEnvelopeCgb() const;
    u8 ReadNoiseControlCgb() const { return frequency_lo; }
    u8 ReadResetCgb() const { return frequency_hi | 0xBF; }

    void WriteSweep(u8 data);
    void WriteWaveControl(u8 data);
    void WriteSoundLength(u8 data);
    void WriteEnvelope(u8 data);
    void WriteFrequencyLow(u8 data) { frequency_lo = data; }
    void WriteReset(u8 data, u32 frame_seq);

    int AccessibleBankOffset() const { return 32 - PlayingBankOffset(); }

    void ClearRegisters();

private:
    const Console console;
    const bool gba_mode;

    static constexpr u8 left_enable_mask = static_cast<u8>(G) << 4;
    static constexpr u8 right_enable_mask = static_cast<u8>(G);

    // Frame Sequencer clock bits
    static constexpr u32 length_clock_bit = 0x01;
    static constexpr u32 envelope_clock_bit = 0x04;
    static constexpr u32 sweep_clock_bit = 0x02;

    // IO registers
    u8 sweep;
    u8 sound_length;
    u8 volume_envelope;
    u8 frequency_lo;
    u8 frequency_hi;

    bool channel_enabled = false;
    u32 period_timer = 0;
    u32 wave_pos = 0;

    // Length Counter
    u32 length_counter = 0;
    bool prev_length_counter_dec = false;

    // Volume Envelope
    u32 volume = 0x00;
    u32 envelope_counter = 0;
    bool prev_envelope_inc = false;
    bool envelope_enabled = false;

    // Frequency Sweep
    u16 shadow_frequency = 0x0000;
    u32 sweep_counter = 0;
    bool prev_sweep_inc = false;
    bool sweep_enabled = false;
    bool performed_negative_calculation = false;

    // Wave Sample Buffer
    u8 current_sample = 0x00;
    u8 last_played_sample = 0x00;
    u32 wave_ram_length_mask = 0x20 - 1;

    // Noise
    u16 lfsr = 0x0001;

    // Duty Cycle
    std::array<unsigned int, 8> duty_cycle;

    void ResetChannel(u32 frame_seq);
    void TimerTick(const std::array<u8, 0x20>& wave_ram);
    void LengthCounterTick(u32 frame_seq);
    void EnvelopeTick(u32 frame_seq);
    void SweepTick(u32 frame_seq);

    void ReloadPeriod();
    void ReloadLengthCounter();
    u16 CalculateSweepFrequency();
    void SetDutyCycle();

    u8 SweepShift() const { return sweep & 0x07; }
    u8 SweepDirection() const { return (sweep & 0x08) >> 3; }
    u8 SweepPeriod() const { return (sweep & 0x70) >> 4; }

    bool SingleBankMode() const { return sweep & 0x20; }
    int PlayingBankOffset() const { return ((sweep & 0x40) >> 6) * 32; }
    bool WaveChannelOn() const { return sweep & 0x80; }

    u8 DutyCycle() const { return (sound_length & 0xC0) >> 6; }

    u8 EnvelopePeriod() const { return volume_envelope & 0x07; }
    u8 EnvelopeDirection() const { return (volume_envelope & 0x08) >> 3; }
    u8 EnvelopeInitialVolume() const { return (volume_envelope & 0xF0) >> 4; }

    int WaveVolumeShift() const { return (volume_envelope & 0x60) >> 5; }
    bool WaveVolumeForce75Percent() const { return volume_envelope & 0x80; }

    u8 ClockDivider() const { return frequency_lo & 0x07; }
    bool CounterWidthHalved() const { return frequency_lo & 0x08; }
    u8 ShiftClock() const { return (frequency_lo & 0xF0) >> 4; }

    bool LengthCounterEnabled() const { return frequency_hi & 0x40; }
    bool ResetEnabled() const { return frequency_hi & 0x80; }
    void ClearReset() { frequency_hi &= 0x7F; }

    int Frequency() const { return frequency_lo | ((frequency_hi & 0x07) << 8); }

    bool FrameSeqBitIsLow(u32 clock_bit, u32 frame_seq) const { return (frame_seq & clock_bit) == 0; }
};

} // End namespace Gb
