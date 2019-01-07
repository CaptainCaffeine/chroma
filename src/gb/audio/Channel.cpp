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

#include "gb/audio/Channel.h"

namespace Gb {

template<Gen G>
Channel<G>::Channel(const Console _console, bool _gba_mode, u8 NRx0, u8 NRx1, u8 NRx2, u8 NRx3, u8 NRx4)
        : console(_console)
        , gba_mode(_gba_mode)
        , sweep(NRx0)
        , sound_length(NRx1)
        , volume_envelope(NRx2)
        , frequency_lo(NRx3)
        , frequency_hi(NRx4) {

    SetDutyCycle();

    if (G == Gen::Square1 && !gba_mode) {
        channel_enabled = true;
    }
}

template<Gen G>
u8 Channel<G>::GenSample() const {
    return duty_cycle[wave_pos] * volume;
}

template<>
u8 Channel<Gen::Wave>::GenSample() const {
    if (WaveVolumeForce75Percent()) {
        return current_sample - (current_sample >> 2);
    } else {
        return current_sample >> volume;
    }
}

template<>
u8 Channel<Gen::Noise>::GenSample() const {
    return ((~lfsr) & 0x0001) * volume;
}

template<Gen G>
void Channel<G>::ReloadPeriod() {
    period_timer = (2048 - Frequency()) << 1;
}

template<>
void Channel<Gen::Wave>::ReloadPeriod() {
    period_timer = 2048 - Frequency();
}

template<>
void Channel<Gen::Noise>::ReloadPeriod() {
    const u32 clock_divider = std::max(ClockDivider() << 1, 1);
    period_timer = clock_divider << (ShiftClock() + 2);
}

// Forward declaration of CalculateSweepFrequency and SweepTick specializations.
template<>
u16 Channel<Gen::Square1>::CalculateSweepFrequency();
template<>
void Channel<Gen::Square1>::SweepTick(u32 frame_seq);

template<Gen G>
void Channel<G>::Update(u32 frame_seq, const std::array<u8, 0x20>& wave_ram) {
    if constexpr (G == Gen::Square1) {
        SweepTick(frame_seq);
    }
    TimerTick(wave_ram);
    LengthCounterTick(frame_seq);
    if constexpr (G != Gen::Wave) {
        EnvelopeTick(frame_seq);
    }
}

template<Gen G>
void Channel<G>::ResetChannel(u32 frame_seq) {
    channel_enabled = true;
    ReloadPeriod();
    ClearReset();

    if constexpr (G == Gen::Square1) {
        shadow_frequency = Frequency();
        sweep_counter = SweepPeriod();
        sweep_enabled = (sweep_counter != 0 && SweepShift() != 0);

        // The next frequency value is calculated immediately when the sweep unit is enabled, but it is not
        // written back to the frequency registers. It will, however, disable the channel if the next value
        // fails the overflow check.
        CalculateSweepFrequency();

        performed_negative_calculation = false;
    }

    if constexpr (G != Gen::Wave) {
        // Initialize volume envelope.
        volume = EnvelopeInitialVolume();
        envelope_counter = EnvelopePeriod();
        envelope_enabled = (envelope_counter != 0);
        if ((EnvelopeDirection() == 0 && volume == 0x00) || (EnvelopeDirection() == 1 && volume == 0x0F)) {
            envelope_enabled = false;
        }
    }

    // If the length counter is zero on trigger, it's set to the maximum value.
    if (length_counter == 0) {
        if constexpr (G == Gen::Wave) {
            length_counter = 256;
        } else {
            length_counter = 64;
        }

        // If we're in the first half of the length counter period and the length counter is enabled, it gets
        // decremented by one.
        if (FrameSeqBitIsLow(length_clock_bit, frame_seq) && LengthCounterEnabled()) {
            length_counter -= 1;
        }
    }

    if constexpr (G == Gen::Noise) {
        lfsr = 0xFFFF;
    }

    if constexpr (G == Gen::Wave) {
        wave_pos = 0;
        channel_enabled = WaveChannelOn();

        // When triggering the wave channel, the first sample to be played is the last sample that was completely
        // played back by the wave channel.
        current_sample = last_played_sample;
    } else {
        // If the current volume is zero, the channel will be disabled immediately after initialization.
        if (volume == 0x00) {
            channel_enabled = false;
        }

        // The wave position is *not* reset to 0 on trigger for the square wave channels.
    }
}

template<Gen G>
void Channel<G>::TimerTick(const std::array<u8, 0x20>&) {
    if (period_timer == 0) {
        wave_pos = (wave_pos + 1) & 0x07;

        ReloadPeriod();
    } else {
        period_timer -= 1;
    }
}

template<>
void Channel<Gen::Wave>::TimerTick(const std::array<u8, 0x20>& wave_ram) {
    if (period_timer == 0) {
        last_played_sample = current_sample;
        wave_pos = (wave_pos + 1) & wave_ram_length_mask;

        const int sample_index = (wave_pos + PlayingBankOffset()) & 0x3F;
        const u8 sample_byte = wave_ram[sample_index >> 1];

        current_sample = (sample_index & 0x01) ? (sample_byte & 0x0F) : (sample_byte >> 4);

        ReloadPeriod();
    } else {
        period_timer -= 1;
    }
}

template<>
void Channel<Gen::Noise>::TimerTick(const std::array<u8, 0x20>&) {
    if (period_timer == 0) {
        if (ShiftClock() < 14) {
            const unsigned int xored_bits = (lfsr ^ (lfsr >> 1)) & 0x0001;
            lfsr >>= 1;
            lfsr |= xored_bits << 14;

            if (CounterWidthHalved()) {
                // Counter is 7 bits instead of 15.
                lfsr &= ~0x0040;
                lfsr |= xored_bits << 6;
            }
        }

        ReloadPeriod();
    } else {
        period_timer -= 1;
    }
}

template<Gen G>
void Channel<G>::LengthCounterTick(u32 frame_seq) {
    const bool length_counter_dec = frame_seq & length_clock_bit;

    if (LengthCounterEnabled() && length_counter > 0) {
        if (!length_counter_dec && prev_length_counter_dec) {
            length_counter -= 1;

            if (length_counter == 0) {
                channel_enabled = false;
            }
        }
    }

    prev_length_counter_dec = length_counter_dec;
}

template<>
void Channel<Gen::Wave>::EnvelopeTick(u32) { }

template<Gen G>
void Channel<G>::EnvelopeTick(u32 frame_seq) {
    const bool envelope_inc = frame_seq & envelope_clock_bit;

    if (envelope_enabled) {
        if (!envelope_inc && prev_envelope_inc) {
            envelope_counter -= 1;

            if (envelope_counter == 0) {
                if (EnvelopeDirection() == 0) {
                    volume -= 1;
                    if (volume == 0x00) {
                        envelope_enabled = false;
                    }
                } else {
                    volume += 1;
                    if (volume == 0x0F) {
                        envelope_enabled = false;
                    }
                }

                envelope_counter = EnvelopePeriod();
            }
        }
    }

    prev_envelope_inc = envelope_inc;
}

template<Gen G>
void Channel<G>::SweepTick(u32) { }

template<>
void Channel<Gen::Square1>::SweepTick(u32 frame_seq) {
    const bool sweep_inc = frame_seq & sweep_clock_bit;

    if (sweep_enabled) {
        if (!sweep_inc && prev_sweep_inc) {
            sweep_counter -= 1;

            if (sweep_counter == 0) {
                shadow_frequency = CalculateSweepFrequency();
                frequency_lo = shadow_frequency & 0x00FF;
                frequency_hi = (frequency_hi & 0xF8) | ((shadow_frequency & 0x0700) >> 8);

                // After writing back the new frequency, it calculates the next value with the new frequency and
                // performs the overflow check again.
                CalculateSweepFrequency();

                // The counter likely stays on zero until the next decrement, but it's easier to just reload it right
                // away with the period + 1.
                sweep_counter = SweepPeriod() + 1;
            }
        }
    }

    prev_sweep_inc = sweep_inc;
}

template<Gen G>
void Channel<G>::ReloadLengthCounter() {
    length_counter = 64 - (sound_length & 0x3F);

    // Clear the written length data.
    sound_length &= 0xC0;
}

template<>
void Channel<Gen::Wave>::ReloadLengthCounter() {
    length_counter = 256 - sound_length;

    // Clear the written length data.
    sound_length = 0x00;
}

template<Gen G>
u16 Channel<G>::CalculateSweepFrequency() { return 0x0000; }

template<>
u16 Channel<Gen::Square1>::CalculateSweepFrequency() {
    u16 frequency_delta = shadow_frequency >> SweepShift();
    if (SweepDirection() == 1) {
        frequency_delta *= -1;
        frequency_delta &= 0x07FF;

        performed_negative_calculation = true;
    }

    u16 new_frequency = (shadow_frequency + frequency_delta) & 0x07FF;

    if (new_frequency > 2047) {
        sweep_enabled = false;
        channel_enabled = false;
    }

    return new_frequency;
}

template<Gen G>
u16 Channel<G>::ReadResetGba() const {
    if constexpr (G == Gen::Noise) {
        return frequency_lo | ((frequency_hi & 0x40) << 8);
    } else {
        return (frequency_hi & 0x40) << 8;
    }
}

template<Gen G>
u8 Channel<G>::ReadSweepCgb() const {
    if constexpr (G == Gen::Square1) {
        return sweep | 0x80;
    } else if constexpr (G == Gen::Wave) {
        return sweep | 0x7F;
    } else {
        return 0;
    }
}

template<Gen G>
u8 Channel<G>::ReadEnvelopeCgb() const {
    if constexpr (G == Gen::Wave) {
        return volume_envelope | 0x9F;
    } else {
        return volume_envelope;
    }
}

template<Gen G>
void Channel<G>::WriteSweep(u8) { }

template<>
void Channel<Gen::Square1>::WriteSweep(u8 data) {
    sweep = data & 0x7F;

    if (SweepPeriod() == 0 || SweepShift() == 0 || (SweepDirection() == 0 && performed_negative_calculation)) {
        sweep_enabled = false;
    }
}

template<Gen G>
void Channel<G>::WriteWaveControl(u8) { }

template<>
void Channel<Gen::Wave>::WriteWaveControl(u8 data) {
    if (gba_mode) {
        sweep = data & 0xE0;
    } else {
        sweep = data & 0x80;
    }

    if (!WaveChannelOn()) {
        channel_enabled = false;
    }

    if (SingleBankMode()) {
        // Wave RAM is operating as a single 64 sample bank.
        wave_ram_length_mask = 0x40 - 1;
    } else {
        // Wave RAM is operating as two 32 sample banks.
        wave_ram_length_mask = 0x20 - 1;
    }
}

template<Gen G>
void Channel<G>::WriteSoundLength(u8 data) {
    if constexpr (G == Gen::Noise) {
        data &= 0x3F;
    }

    sound_length = data;
    ReloadLengthCounter();

    if constexpr (G == Gen::Square1 || G == Gen::Square2) {
        SetDutyCycle();
    }
}

template<Gen G>
void Channel<G>::WriteEnvelope(u8 data) {
    volume_envelope = data;

    if (EnvelopeInitialVolume() == 0) {
        channel_enabled = false;
    }
}

template<>
void Channel<Gen::Wave>::WriteEnvelope(u8 data) {
    volume_envelope = data & 0xE0;
    volume = (WaveVolumeShift()) ? WaveVolumeShift() - 1 : 4;
}

template<Gen G>
void Channel<G>::WriteReset(u8 data, u32 frame_seq) {
    const bool length_counter_was_enabled = LengthCounterEnabled();
    if constexpr (G == Gen::Noise) {
        frequency_hi = data & 0xC0;
    } else {
        frequency_hi = data & 0xC7;
    }

    // If the length counter gets enabled while we're in the first half of the length counter period,
    // it gets decremented by one.
    if (FrameSeqBitIsLow(length_clock_bit, frame_seq)
            && !length_counter_was_enabled
            && LengthCounterEnabled()
            && length_counter > 0) {

        length_counter -= 1;

        if (length_counter == 0) {
            channel_enabled = false;
        }
    }

    if (ResetEnabled()) {
        ResetChannel(frame_seq);
    }
}

template<Gen G>
void Channel<G>::ClearRegisters() {
    sweep = 0x00;
    sound_length = 0x00;
    volume_envelope = 0x00;
    frequency_lo = 0x00;
    frequency_hi = 0x00;

    volume = 0x00;
    envelope_counter = 0;
    prev_envelope_inc = false;
    envelope_enabled = false;

    shadow_frequency = 0x0000;
    sweep_counter = 0;
    prev_sweep_inc = false;
    sweep_enabled = false;
    performed_negative_calculation = false;

    wave_pos = 0;
    current_sample = 0x00;
    last_played_sample = 0x00;

    lfsr = 0x0000;

    // On DMG, the length counters are unaffected by power state.
    if (console != Console::DMG) {
        length_counter = 0x00;
    }

    channel_enabled = false;
}

template<>
void Channel<Gen::Wave>::SetDutyCycle() { }

template<>
void Channel<Gen::Noise>::SetDutyCycle() { }

template<Gen G>
void Channel<G>::SetDutyCycle() {
    switch (DutyCycle()) {
    case 0x00:
        duty_cycle = {{0, 0, 0, 0, 0, 0, 0, 1}};
        break;
    case 0x01:
        duty_cycle = {{1, 0, 0, 0, 0, 0, 0, 1}};
        break;
    case 0x02:
        duty_cycle = {{1, 0, 0, 0, 0, 1, 1, 1}};
        break;
    case 0x03:
        duty_cycle = {{0, 1, 1, 1, 1, 1, 1, 0}};
        break;
    default:
        break;
    }
}

// Explicit instantiations of each Channel template to avoid needing to place all functions in the header file.
template class Channel<Gen::Square1>;
template class Channel<Gen::Square2>;
template class Channel<Gen::Wave>;
template class Channel<Gen::Noise>;

} // End namespace Gb
