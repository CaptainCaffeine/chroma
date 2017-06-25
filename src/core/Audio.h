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

#pragma once

#include <array>
#include <vector>

#include "common/CommonTypes.h"

namespace Core {

class Memory;

class Audio {
public:
    void UpdateAudio();

    void LinkToMemory(Memory* memory) { mem = memory; }
    bool IsPoweredOn() const { return audio_on; }

    std::vector<u8> sample_buffer;

    // ******** Audio I/O registers ********
    // NR10 register: 0xFF10
    //     bit 6-4: Sweep Time (n/128Hz)
    //     bit 3:   Sweep Direction (0=increase, 1=decrease)
    //     bit 2-0: Sweep Shift
    u8 sweep_channel1 = 0x00;
    // NR11 register: 0xFF11
    //     bit 7-6: Wave Pattern Duty
    //     bit 5-0: Sound Length (Write Only)
    u8 sound_length_channel1 = 0x80;
    // NR12 register: 0xFF12
    //     bit 7-4: Initial Volume of Envelope
    //     bit 3:   Envelope Direction (0=decrease, 1=increase)
    //     bit 2-0: Step Time (n/64 seconds)
    u8 envelope_channel1 = 0xF3;
    // NR13 register: 0xFF13 (Write Only)
    u8 frequency_lo_channel1 = 0xFF;
    // NR14 register: 0xFF14
    //     bit 7:   Reset (1=Restart Sound) (Write Only)
    //     bit 6:   Timed Mode (0=continuous, 1=stop when length expires)
    //     bit 2-0: Frequency High 3 Bits (Write Only)
    u8 frequency_hi_channel1 = 0x00;

    // NR21 register: 0xFF16
    //     bit 7-6: Wave Pattern Duty
    //     bit 5-0: Sound Length (Write Only)
    u8 sound_length_channel2 = 0x00;
    // NR22 register: 0xFF17
    //     bit 7-4: Initial Volume of Envelope
    //     bit 3:   Envelope Direction (0=decrease, 1=increase)
    //     bit 2-0: Step Time (n/64 seconds)
    u8 envelope_channel2 = 0x00;
    // NR23 register: 0xFF18
    u8 frequency_lo_channel2 = 0xFF;
    // NR24 register: 0xFF19
    //     bit 7:   Reset (1=Restart Sound) (Write Only)
    //     bit 6:   Timed Mode (0=continuous, 1=stop when length expires)
    //     bit 2-0: Frequency High 3 Bits (Write Only)
    u8 frequency_hi_channel2 = 0x00;

    // NR30 register: 0xFF1A
    //     bit 7: Channel On/Off (0=Off, 1=On)
    u8 sound_on_channel3 = 0x00;
    // NR31 register: 0xFF1B
    //     bit 7-0: Sound Length
    u8 sound_length_channel3 = 0xFF;
    // NR32 register: 0xFF1C
    //     bit 6-5: Output Level (0=Silent, 1=unshifted/100%, 2=shifted right once/50%, 3=shifted right twice/25%)
    u8 output_channel3 = 0x00;
    // NR33 register: 0xFF1D
    u8 frequency_lo_channel3 = 0xFF;
    // NR34 register: 0xFF1E
    //     bit 7:   Reset (1=Restart Sound) (Write Only)
    //     bit 6:   Timed Mode (0=continuous, 1=stop when length expires)
    //     bit 2-0: Frequency High 3 Bits (Write Only)
    u8 frequency_hi_channel3 = 0x00;

    // NR41 register: 0xFF20
    //     bit 5-0: Sound Length
    u8 sound_length_channel4 = 0x1F;
    // NR42 register: 0xFF21
    //     bit 7-4: Initial Volume of Envelope
    //     bit 3:   Envelope Direction (0=decrease, 1=increase)
    //     bit 2-0: Step Time (n/64 seconds)
    u8 envelope_channel4 = 0x00;
    // NR43 register: 0xFF22
    //     bit 7-4: Shift Clock Frequency
    //     bit 3:   Counter Step (0=15 bits, 1=7 bits)
    //     bit 2-0: Clock Divider
    u8 poly_counter_channel4 = 0x00;
    // NR44 register: 0xFF23
    //     bit 7: Reset (1=Restart Sound) (Write Only)
    //     bit 6: Timed Mode (0=continuous, 1=stop when length expires)
    u8 counter_channel4 = 0x00;

    // NR50 register: 0xFF24
    //     bit 7:   Output Vin to SO2 (1=Enable)
    //     bit 6-4: SO2 Output Level
    //     bit 3:   Output Vin to SO1 (1=Enable)
    //     bit 2-0: SO1 Output Level
    u8 volume = 0x77;
    // NR51 register: 0xFF25
    //     bit 7: Output Channel 4 to SO2
    //     bit 6: Output Channel 3 to SO2
    //     bit 5: Output Channel 2 to SO2
    //     bit 4: Output Channel 1 to SO2
    //     bit 3: Output Channel 4 to SO1
    //     bit 2: Output Channel 3 to SO1
    //     bit 1: Output Channel 2 to SO1
    //     bit 0: Output Channel 1 to SO1
    u8 sound_select = 0xF3;
    // NR52 register: 0xFF26
    //     bit 7: Master Sound On/Off (0=Off)
    //     bit 3: Channel 4 On (Read Only)
    //     bit 2: Channel 3 On (Read Only)
    //     bit 1: Channel 2 On (Read Only)
    //     bit 0: Channel 1 On (Read Only)
    u8 sound_on = 0x81;

    // Wave Pattern RAM: 0xFF30-0xFF3F
    std::array<u8, 0x10> wave_ram{{0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
                                   0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF}};
private:
    Memory* mem;

    bool audio_on = true;
    void UpdatePowerOnState();
};

} // End namespace Core