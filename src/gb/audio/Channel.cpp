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

#include "gb/audio/Channel.h"
#include "gb/audio/Audio.h"

namespace Gb {

Channel::Channel(Generator gen, std::array<u8, 0x10>& wave_ram_ref, u8 NRx0, u8 NRx1, u8 NRx2, u8 NRx3, u8 NRx4)
        : sweep(NRx0)
        , sound_length(NRx1)
        , volume_envelope(NRx2)
        , frequency_lo(NRx3)
        , frequency_hi(NRx4)
        , channel_enabled(gen == Generator::Square1)
        , wave_ram(wave_ram_ref)
        , gen_type(gen)
        , left_enable_mask(static_cast<u8>(gen_type) << 4)
        , right_enable_mask(static_cast<u8>(gen_type)) {}

u8 Channel::GenSample() const {
    if (gen_type == Generator::Wave) {
        u8 volume_shift = (WaveVolumeShift()) ? WaveVolumeShift() - 1 : 4;
        return current_sample >> volume_shift;
    } else if (gen_type == Generator::Noise) {
        return ((~lfsr) & 0x0001) * volume;
    } else {
        return DutyCycle((sound_length & 0xC0) >> 6)[wave_pos] * volume;
    }
}

void Channel::CheckTrigger(const Console console) {
    if (frequency_hi & 0x80) {
        // Clear reset flag.
        frequency_hi &= 0x7F;

        channel_enabled = true;
        ReloadPeriod();

        if (gen_type == Generator::Square1) {
            shadow_frequency = frequency_lo | ((frequency_hi & 0x07) << 8);
            sweep_counter = SweepPeriod();
            sweep_enabled = (sweep_counter != 0 && SweepShift() != 0);

            // The next frequency value is calculated immediately when the sweep unit is enabled, but it is not
            // written back to the frequency registers. It will, however, disable the channel if the next value
            // fails the overflow check.
            CalculateSweepFrequency();

            performed_negative_calculation = false;
        }

        if (gen_type != Generator::Wave) {
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
            if (gen_type == Generator::Wave) {
                length_counter = 256;
            } else {
                length_counter = 64;
            }

            // If we're in the first half of the length counter period and the length counter is enabled, it gets
            // decremented by one.
            if (FrameSeqBitIsLow(length_clock_bit) && LengthCounterEnabled()) {
                length_counter -= 1;
            }
        }

        if (gen_type == Generator::Noise) {
            lfsr = 0xFFFF;
        }

        if (gen_type == Generator::Wave) {
            if (console == Console::DMG && reading_sample) {
                CorruptWaveRAM();
            }

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
}

void Channel::TimerTick() {
    reading_sample = false;

    if (period_timer == 0) {
        if (gen_type == Generator::Wave) {
            last_played_sample = current_sample;
            wave_pos = (wave_pos + 1) & 0x1F;
            current_sample = GetNextSample();
            reading_sample = true;
        } else if (gen_type == Generator::Noise) {
            if (ShiftClock() < 14) {
                const unsigned int xored_bits = (lfsr ^ (lfsr >> 1)) & 0x0001;
                lfsr >>= 1;
                lfsr |= xored_bits << 14;

                if (frequency_lo & 0x08) {
                    lfsr &= ~0x0040;
                    lfsr |= xored_bits << 6;
                }
            }
        } else {
            wave_pos = (wave_pos + 1) & 0x07;
        }

        ReloadPeriod();
    } else {
        period_timer -= 1;
    }
}

void Channel::LengthCounterTick() {
    bool length_counter_dec = audio->frame_seq_counter & length_clock_bit;

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

void Channel::EnvelopeTick() {
    bool envelope_inc = audio->frame_seq_counter & envelope_clock_bit;

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

void Channel::SweepTick() {
    bool sweep_inc = audio->frame_seq_counter & sweep_clock_bit;

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

void Channel::ReloadPeriod() {
    if (gen_type == Generator::Wave) {
        period_timer = (2048 - (frequency_lo | ((frequency_hi & 0x07) << 8)));
    } else if (gen_type == Generator::Noise) {
        unsigned int clock_divisor = (frequency_lo & 0x07) << 1;
        if (clock_divisor == 0) {
            clock_divisor = 1;
        }

        period_timer = clock_divisor << (ShiftClock() + 2);
    } else {
        period_timer = (2048 - (frequency_lo | ((frequency_hi & 0x07) << 8))) << 1;
    }
}

void Channel::ReloadLengthCounter() {
    if (gen_type == Generator::Wave) {
        length_counter = 256 - sound_length;

        // Clear the written length data.
        sound_length = 0x00;
    } else {
        length_counter = 64 - (sound_length & 0x3F);

        // Clear the written length data.
        sound_length &= 0xC0;
    }
}

void Channel::ExtraLengthClocking(u8 new_frequency_hi) {
    if (FrameSeqBitIsLow(length_clock_bit)
            && !LengthCounterEnabled()
            && (new_frequency_hi & 0x40)
            && length_counter > 0) {

        length_counter -= 1;

        if (length_counter == 0 && !(new_frequency_hi & 0x80)) {
            channel_enabled = false;
        }
    }
}

u16 Channel::CalculateSweepFrequency() {
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

void Channel::SweepWriteHandler() {
    if (SweepPeriod() == 0 || SweepShift() == 0 || (SweepDirection() == 0 && performed_negative_calculation)) {
        sweep_enabled = false;
    }
}

void Channel::ClearRegisters(const Console console) {
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
    reading_sample = false;
    current_sample = 0x00;
    last_played_sample = 0x00;

    lfsr = 0x0000;

    // On DMG, the length counters are unaffected by power state.
    if (console != Console::DMG) {
        length_counter = 0x00;
    }

    channel_enabled = false;
}

void Channel::CorruptWaveRAM() {
    // On DMG, if the wave channel is triggered while a sample is being read from the wave RAM, the first few bytes
    // of wave RAM get corrupted.
    if (wave_pos < 4) {
        // If the sample being read is one of the first four bytes, then the first byte is set to the byte being read.
        wave_ram[0] = current_sample;
    } else {
        // Otherwise, the first 4 bytes are overwritten with 4 bytes in the 4-aligned block around the byte being read.
        unsigned int copy_pos = (wave_pos / 4) * 4;
        for (int i = 0; i < 4; ++i) {
            wave_ram[i] = wave_ram[copy_pos + i];
        }
    }
}

bool Channel::FrameSeqBitIsLow(unsigned int clock_bit) const {
    return (audio->frame_seq_counter & clock_bit) == 0;
}

std::array<unsigned int, 8> Channel::DutyCycle(const u8 cycle) const {
    switch(cycle) {
    case 0x00:
        return {{0, 0, 0, 0, 0, 0, 0, 1}};
    case 0x01:
        return {{1, 0, 0, 0, 0, 0, 0, 1}};
    case 0x02:
        return {{1, 0, 0, 0, 0, 1, 1, 1}};
    case 0x03:
        return {{0, 1, 1, 1, 1, 1, 1, 0}};
    default:
        return {{0, 0, 0, 0, 0, 0, 0, 0}};
    }
}

} // End namespace Gb
