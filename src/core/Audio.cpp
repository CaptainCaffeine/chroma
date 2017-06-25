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
    UpdatePowerOnState();

    if (!audio_on) {
        return;
    }
}

void Audio::UpdatePowerOnState() {
    bool audio_power_on = sound_on & 0x80;
    if (audio_power_on != audio_on) {
        audio_on = audio_power_on;

        if (!audio_on) {
            // Clear all sound registers.
            sweep_channel1 = 0x00;
            envelope_channel1 = 0x00;
            frequency_lo_channel1 = 0x00;
            frequency_hi_channel1 = 0x00;

            envelope_channel2 = 0x00;
            frequency_lo_channel2 = 0x00;
            frequency_hi_channel2 = 0x00;

            sound_on_channel3 = 0x00;
            output_channel3 = 0x00;
            frequency_lo_channel3 = 0x00;
            frequency_hi_channel3 = 0x00;

            envelope_channel4 = 0x00;
            poly_counter_channel4 = 0x00;
            counter_channel4 = 0x00;

            volume = 0x00;
            sound_select = 0x00;
            sound_on = 0x00;

            if (mem->console != Console::DMG) {
                // On DMG, the length counters are unaffected by power state.
                // TODO: Is the wave duty preserved too? Or just the length counter?
                sound_length_channel1 = 0x00;
                sound_length_channel2 = 0x00;
                sound_length_channel3 = 0x00;
                sound_length_channel4 = 0x00;
            }
        }
    }
}

} // End namespace Core
