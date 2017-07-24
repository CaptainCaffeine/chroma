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

#include "core/Channel.h"

namespace Core {

void Channel::CheckTrigger() {
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

        // Initialize volume envelope.
        volume = EnvelopeInitialVolume();
        envelope_counter = EnvelopePeriod();
        envelope_enabled = (envelope_counter != 0);
        if ((EnvelopeDirection() == 0 && volume == 0x00) || (EnvelopeDirection() == 1 && volume == 0x0F)) {
            envelope_enabled = false;
        }

        // If the length counter is zero on trigger, it's set to the maximum value.
        if (length_counter == 0) {
            length_counter = 64;
        }

        // If the current volume is zero, the channel will be disabled immediately after initialization.
        if (volume == 0x00) {
            channel_enabled = false;
        }
    }
}

void Channel::TimerTick() {
    if (period_timer == 0) {
        duty_pos += 1;
        duty_pos &= 0x07;

        ReloadPeriod();
    } else {
        period_timer -= 1;
    }
}

void Channel::LengthCounterTick(const unsigned int frame_seq_counter) {
    bool length_counter_dec = frame_seq_counter & 0x01;

    if ((frequency_hi & 0x40) && length_counter > 0) {
        if (!length_counter_dec && prev_length_counter_dec) {
            length_counter -= 1;

            if (length_counter == 0) {
                channel_enabled = false;
            }
        }
    }

    prev_length_counter_dec = length_counter_dec;
}

void Channel::EnvelopeTick(const unsigned int frame_seq_counter) {
    bool envelope_inc = frame_seq_counter & 0x04;

    if (envelope_enabled && channel_enabled) {
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

void Channel::SweepTick(const unsigned int frame_seq_counter) {
    bool sweep_inc = frame_seq_counter & 0x02;

    //if (sweep_enabled && channel_enabled) {
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

void Channel::ReloadLengthCounter() {
    length_counter = 64 - (sound_length & 0x3F);

    // Clear the written length data.
    sound_length &= 0xC0;
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
    volume_envelope = 0x00;
    frequency_lo = 0x00;
    frequency_hi = 0x00;

    if (console == Console::DMG) {
        // On DMG, the length counters are unaffected by power state.
        sound_length &= 0x3F;
    } else {
        sound_length = 0x00;
    }

    channel_enabled = false;
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

} // End namespace Core
