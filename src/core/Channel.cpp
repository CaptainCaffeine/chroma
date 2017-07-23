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

        // Initialize volume envelope.
        volume = EnvelopeInitialVolume();
        envelope_counter = EnvelopeStep();
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

                envelope_counter = EnvelopeStep();
            }
        }
    }

    prev_envelope_inc = envelope_inc;
}

void Channel::ReloadLengthCounter() {
    length_counter = 64 - (sound_length & 0x3F);

    // Clear the written length data.
    sound_length &= 0xC0;
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
