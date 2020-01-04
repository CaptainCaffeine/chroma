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

#include <ctime>

#include "common/CommonFuncs.h"
#include "gba/hardware/Rtc.h"
#include "gba/core/Core.h"
#include "gba/cpu/Disassembler.h"

namespace Gba {

Rtc::Rtc(Core& _core)
        : core(_core) {}

u16 Rtc::UpdateState(u16 data, bool write) {
    if (ChipSelectLow(data)) {
        if (!serial_bitstream.empty()) {
            if (command_state == CommandState::Writing) {
                WriteRegister();
            } else if (command_state == CommandState::Ready) {
                // Size of an RTC command must be at least one byte.
                core.disasm->LogAlways("Size of an RTC command was less than 8 bits: {}.\n", serial_bitstream.size());
            }

            serial_bitstream.clear();
        }

        transfer_state = TransferState::Ready;
        command_state = CommandState::Ready;
    }

    switch (transfer_state) {
    case TransferState::Ready:
        if (ClockHigh(data)) {
            transfer_state = TransferState::Starting;
        }
        break;
    case TransferState::Starting:
        if (ChipSelectHigh(data) && ClockHigh(data)) {
            transfer_state = TransferState::ClockHigh;
        }
        break;
    case TransferState::ClockHigh:
        if (ClockLow(data)) {
            transfer_state = TransferState::ClockLow;
        }
        break;
    case TransferState::ClockLow:
        if (ClockHigh(data)) {
            transfer_state = TransferState::ClockHigh;

            if (command_state == CommandState::Reading) {
                // Update SIO bit.
                data = SetSio(data, ReadRegister());
            }

            if (write) {
                // Add SIO bit to buffer.

                // Will have to check if games always write the same data bit when setting SCK high,
                // or if they always write 0 or something.
                serial_bitstream.push_back(GetSio(data));

                if (command_state == CommandState::Ready && serial_bitstream.size() == 8) {
                    ParseCommand();
                }
            }
        }
        break;
    }

    return data;
}

static constexpr u8 BitSwap(u8 value) noexcept {
    const u8 swapped_value =   ((value & 0x01) << 7)
                             | ((value & 0x02) << 5)
                             | ((value & 0x04) << 3)
                             | ((value & 0x08) << 1)
                             | ((value & 0x10) >> 1)
                             | ((value & 0x20) >> 3)
                             | ((value & 0x40) >> 5)
                             | ((value & 0x80) >> 7);

    return swapped_value;
}

void Rtc::ParseCommand() {
    u8 command = GetSerialBitstreamByte(0);
    serial_bitstream.clear();

    // All valid commands must begin or end with 0x6/0b0110.
    if ((command & 0xF) == 0x6) {
        // Bitswap the command byte to be MSB-first.
        command = BitSwap(command);
    } else if ((command >> 4) != 0x6) {
        core.disasm->LogAlways("RTC command did not start or end with 0x6: 0x{:0>2X}.\n", command);
        return;
    }

    const bool read_access = command & 0x1;
    const CommandReg cmd_reg = static_cast<CommandReg>((command >> 1) & 0x7);

    if (!ValidRtcRegister(cmd_reg)) {
        core.disasm->LogAlways("Invalid RTC register: 0x{:X}.\n", cmd_reg);
        return;
    }

    if (cmd_reg == CommandReg::ForceReset) {
        // Reset all RTC registers to 0, except for day & month which get reset to 1.
        date_time[TimeReg::Year]    = 0;
        date_time[TimeReg::Month]   = 1;
        date_time[TimeReg::Day]     = 1;
        date_time[TimeReg::Weekday] = 0;
        date_time[TimeReg::Hour]    = 0;
        date_time[TimeReg::Minute]  = 0;
        date_time[TimeReg::Second]  = 0;

        return;
    } else if (cmd_reg == CommandReg::ForceIrq) {
        // TODO
        return;
    }

    reg_being_accessed = cmd_reg;
    bits_read = 0;
    if (read_access) {
        command_state = CommandState::Reading;

        if (cmd_reg == CommandReg::Time) {
            bits_read = time_regs_offset * 8;
        }
    } else {
        command_state = CommandState::Writing;
    }
}

int Rtc::ReadRegister() {
    UpdateTime();

    switch (reg_being_accessed) {
    case CommandReg::Control:
        if (bits_read >= 8) {
            core.disasm->LogAlways("Reading OOB bit from Control.\n");
            return 0;
        }

        return (control >> bits_read++) & 0x1;
    case CommandReg::DateTime:
    case CommandReg::Time: {
        if (bits_read >= 8 * date_time.size()) {
            core.disasm->LogAlways("Reading OOB bit from DateTime.\n");
            return 0;
        }

        const int next_bit = (date_time[bits_read / 8].Read() >> (bits_read % 8)) & 0x1;
        bits_read += 1;
        return next_bit;
    }
    default:
        return 0;
    }
}

std::string Rtc::PrintTimeReg(TimeReg reg) {
    switch (reg) {
    case TimeReg::Year:
        return "Year";
    case TimeReg::Month:
        return "Month";
    case TimeReg::Day:
        return "Day";
    case TimeReg::Weekday:
        return "Weekday";
    case TimeReg::Hour:
        return "Hour";
    case TimeReg::Minute:
        return "Minute";
    case TimeReg::Second:
        return "Second";
    }
    return "Bad TimeReg!";
}

void Rtc::WriteRegister() {
    switch (reg_being_accessed) {
    case CommandReg::Control:
        core.disasm->LogAlways("Writing Control with 0x{:X}\n", GetSerialBitstreamByte(0));
        WriteControl(GetSerialBitstreamByte(0));
        break;
    case CommandReg::DateTime:
        for (u32 i = 0; i < date_time.size(); ++i) {
            core.disasm->LogAlways("Writing {} with 0x{:X}\n",
                                   PrintTimeReg(static_cast<TimeReg>(i)),
                                   GetSerialBitstreamByte(i));
            date_time[i].Write(GetSerialBitstreamByte(i));
        }

        if (date_time[TimeReg::Month] == 0) {
            date_time[TimeReg::Month] = 1;
        }

        if (date_time[TimeReg::Day] == 0) {
            date_time[TimeReg::Day] = 1;
        }
        break;
    case CommandReg::Time:
        for (u32 i = 0; i < 3; ++i) {
            core.disasm->LogAlways("Writing {} with 0x{:X}\n",
                                   PrintTimeReg(static_cast<TimeReg>(i + time_regs_offset)),
                                   GetSerialBitstreamByte(i));
            date_time[i + time_regs_offset].Write(GetSerialBitstreamByte(i));
        }
        break;
    default:
        break;
    }
}

void Rtc::WriteControl(u8 data) {
    const bool was_24h_mode = TwentyFourHourMode();
    control.Write(data);

    if (was_24h_mode && !TwentyFourHourMode()) {
        // Adjust to 12-hour time.
        if (IsAfternoon()) {
            int hour = ConvertFromBcd(date_time[TimeReg::Hour] & ~am_pm_flag);
            hour -= 12;
            date_time[TimeReg::Hour] = ConvertToBcd(hour) | am_pm_flag;
        }

        date_time[TimeReg::Hour].write_mask = 0x00BF;
    } else if (!was_24h_mode && TwentyFourHourMode()) {
        // Adjust to 24-hour time.
        if (IsAfternoon()) {
            int hour = ConvertFromBcd(date_time[TimeReg::Hour] & ~am_pm_flag);
            hour += 12;
            date_time[TimeReg::Hour] = ConvertToBcd(hour) | am_pm_flag;
        }

        date_time[TimeReg::Hour].write_mask = 0x003F;
    }
}

u8 Rtc::GetSerialBitstreamByte(int byte_index) const {
    u8 value = 0;
    for (int i = 0; i < 8; ++i) {
        const u32 bitstream_index = i + byte_index * 8;
        if (bitstream_index >= serial_bitstream.size()) {
            core.disasm->LogAlways("The game did not write enough data to the RTC."
                                   "Received bits: {}, attempted reading bit {}.\n",
                                   serial_bitstream.size(), bitstream_index);
            break;
        }

        value |= serial_bitstream[i + byte_index * 8] << i;
    }

    return value;
}

void Rtc::UpdateTime() {
    const std::time_t current_time = std::time(nullptr);
    const std::tm* local_date_time = std::localtime(&current_time);

    date_time[TimeReg::Year]    = ConvertToBcd(local_date_time->tm_year - 100);
    date_time[TimeReg::Month]   = ConvertToBcd(local_date_time->tm_mon + 1);
    date_time[TimeReg::Day]     = ConvertToBcd(local_date_time->tm_mday);
    date_time[TimeReg::Weekday] = ConvertToBcd(local_date_time->tm_wday);
    date_time[TimeReg::Hour]    = ConvertToBcd(local_date_time->tm_hour % (TwentyFourHourMode() ? 24 : 12));
    date_time[TimeReg::Minute]  = ConvertToBcd(local_date_time->tm_min);
    date_time[TimeReg::Second]  = ConvertToBcd(local_date_time->tm_sec);

    if (local_date_time->tm_hour >= 12) {
        date_time[TimeReg::Hour] |= am_pm_flag;
    }
}

bool Rtc::ValidRtcRegister(CommandReg cmd_reg) {
    switch (cmd_reg) {
    case CommandReg::ForceReset:
    case CommandReg::Control:
    case CommandReg::DateTime:
    case CommandReg::Time:
    case CommandReg::ForceIrq:
        return true;
    default:
        return false;
    }
}

} // End namespace Gba
