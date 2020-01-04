// This file is a part of Chroma.
// Copyright (C) 2019-2020 Matthew Murray
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

#include <vector>
#include <array>

#include "common/CommonTypes.h"
#include "gba/memory/IOReg.h"

namespace Gba {

class Core;

class Rtc {
public:
    Rtc(Core& _core);

    enum Pin : u16 {SCK = 0x01,
                    SIO = 0x02,
                    CS  = 0x04};

    u16 UpdateState(u16 data, bool write);

private:
    enum class TransferState {Ready,
                              Starting,
                              ClockHigh,
                              ClockLow};

    enum class CommandState {Ready,
                             Writing,
                             Reading};

    enum CommandReg {ForceReset = 0,
                     Control    = 1,
                     DateTime   = 2,
                     Time       = 3,
                     ForceIrq   = 6};

    enum TimeReg {Year    = 0,
                  Month   = 1,
                  Day     = 2,
                  Weekday = 3,
                  Hour    = 4,
                  Minute  = 5,
                  Second  = 6};

    Core& core;

    TransferState transfer_state = TransferState::Ready;
    CommandState command_state = CommandState::Ready;
    CommandReg reg_being_accessed;

    std::vector<u8> serial_bitstream;

    IOReg control = {0x0000, 0x006A, 0x006A};

    std::array<IOReg, 7> date_time{{
        {0x0000, 0x00FF, 0x00FF}, // Year
        {0x0001, 0x001F, 0x001F}, // Month
        {0x0001, 0x003F, 0x003F}, // Day
        {0x0000, 0x0003, 0x0003}, // Weekday
        {0x0000, 0x00BF, 0x003F}, // Hour - AM/PM flag read-only in 24h mode
        {0x0000, 0x007F, 0x007F}, // Minute
        {0x0000, 0x007F, 0x007F}  // Second
    }};

    static constexpr int am_pm_flag = 0x80;
    static constexpr int time_regs_offset = 4;
    u32 bits_read = 0;

    bool PeriodicIrqEnabled() const { return control & 0x08; }
    bool TwentyFourHourMode() const { return control & 0x40; }

    bool IsAfternoon() const { return date_time[TimeReg::Hour] & am_pm_flag; }

    void ParseCommand();

    int ReadRegister();
    void WriteRegister();
    void WriteControl(u8 data);

    u8 GetSerialBitstreamByte(int byte_index) const;

    void UpdateTime();

    static bool ClockLow(u16 data) { return (data & Pin::SCK) == 0; }
    static bool ClockHigh(u16 data) { return (data & Pin::SCK) != 0; }
    static bool ChipSelectLow(u16 data) { return (data & Pin::CS) == 0; }
    static bool ChipSelectHigh(u16 data) { return (data & Pin::CS) != 0; }

    static u8 GetSio(u16 data) { return (data & Pin::SIO) >> 1; }
    static u16 SetSio(u16 data, int new_sio) { return (data & ~Pin::SIO) | (new_sio << 1); }

    static bool ValidRtcRegister(CommandReg cmd_reg);

    static int ConvertToBcd(int value) { return ((value / 10) << 4) + (value % 10); }
    static int ConvertFromBcd(int value) { return ((value >> 4) * 10) + (value & 0xF); }

    static std::string PrintTimeReg(TimeReg reg);
};

} // End namespace Gba
