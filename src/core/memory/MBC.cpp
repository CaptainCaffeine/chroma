// This file is a part of Chroma.
// Copyright (C) 2016 Matthew Murray
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

#include "core/memory/Memory.h"
#include "core/RTC.h"

namespace Core {

u8 Memory::ReadExternalRAM(const u16 addr) const {
    if (ext_ram_enabled) {
        u16 adjusted_addr = addr - 0xA000 + 0x2000 * (ram_bank_num & (num_ram_banks - 1));

        switch (mbc_mode) {
        case MBC::MBC1:
        case MBC::MBC1M:
            // Out of bounds reads return 0xFF.
            if (adjusted_addr < ext_ram.size()) {
                return ext_ram[adjusted_addr];
            } else {
                return 0xFF;
            }
        case MBC::MBC2:
            // MBC2 RAM range is only A000-A1FF.
            if (adjusted_addr < ext_ram.size()) {
                return ext_ram[adjusted_addr] & 0xF0;
            } else {
                return 0xFF;
            }
        case MBC::MBC3:
            // Bit 4 of the RAM bank number is set for RTC registers, and unset for RAM banks.
            if (ram_bank_num & 0x08) {
                // Any address in the range will work to write the RTC registers.
                switch (ram_bank_num) {
                case 0x08:
                    return rtc->GetLatchedTime<RTC::Seconds>();
                case 0x09:
                    return rtc->GetLatchedTime<RTC::Minutes>();
                case 0x0A:
                    return rtc->GetLatchedTime<RTC::Hours>();
                case 0x0B:
                    return rtc->GetLatchedTime<RTC::Days>();
                case 0x0C:
                    return rtc->GetFlags() | 0x3E;
                default:
                    // I'm assuming an invalid register value (0x0D-0x0F) returns 0xFF, needs confirmation though.
                    return 0xFF;
                }
            } else {
                // Out of bounds reads return 0xFF.
                if (adjusted_addr < ext_ram.size()) {
                    return ext_ram[adjusted_addr];
                } else {
                    return 0xFF;
                }
            }
        case MBC::MBC5:
            if (rumble_present) {
                // Carts with rumble cannot use bit 4 of the RAM bank register for bank selection.
                adjusted_addr = addr - 0xA000 + 0x2000 * ((ram_bank_num & 0x07) & (num_ram_banks - 1));
            }

            // Out of bounds reads return 0xFF.
            if (adjusted_addr < ext_ram.size()) {
                return ext_ram[adjusted_addr];
            } else {
                return 0xFF;
            }

        default:
            return 0xFF;
        }
    } else {
        // Reads from this region when the RAM banks are disabled or not present return 0xFF.
        return 0xFF;
    }
}

void Memory::WriteExternalRAM(const u16 addr, const u8 data) {
    // Writes are ignored if external RAM is disabled or not present.
    if (ext_ram_enabled) {
        u16 adjusted_addr = addr - 0xA000 + 0x2000 * (ram_bank_num & (num_ram_banks - 1));

        switch (mbc_mode) {
        case MBC::MBC1:
        case MBC::MBC1M:
            // Ignore out-of-bounds writes.
            if (adjusted_addr < ext_ram.size()) {
                ext_ram[adjusted_addr] = data;
            }
            break;
        case MBC::MBC2:
            // MBC2 RAM range is only A000-A1FF. Only the lower nibble of the bytes in this region are used.
            if (adjusted_addr < ext_ram.size()) {
                ext_ram[adjusted_addr] = data & 0x0F;
            }
            break;
        case MBC::MBC3:
            // Bit 4 of the RAM bank number is set for RTC registers, and unset for RAM banks.
            if (ram_bank_num & 0x08) {
                // Any address in the range will work to write the RTC registers.
                switch (ram_bank_num) {
                case 0x08:
                    rtc->SetTime<RTC::Seconds>(data);
                    break;
                case 0x09:
                    rtc->SetTime<RTC::Minutes>(data);
                    break;
                case 0x0A:
                    rtc->SetTime<RTC::Hours>(data);
                    break;
                case 0x0B:
                    rtc->SetTime<RTC::Days>(data);
                    break;
                case 0x0C:
                    rtc->SetFlags(data);
                    break;
                default:
                    // I'm assuming an invalid register value (0x0D-0x0F) is just ignored.
                    break;
                }
            } else {
                // Ignore out-of-bounds writes.
                if (adjusted_addr < ext_ram.size()) {
                    ext_ram[adjusted_addr] = data;
                }
            }
            break;
        case MBC::MBC5:
            if (rumble_present) {
                // Carts with rumble cannot use bit 4 of the RAM bank register for bank selection.
                adjusted_addr = addr - 0xA000 + 0x2000 * ((ram_bank_num & 0x07) & (num_ram_banks - 1));
            }

            // Ignore out-of-bounds writes.
            if (adjusted_addr < ext_ram.size()) {
                ext_ram[adjusted_addr] = data;
            }
            break;

        default:
            break;
        }
    }
}

void Memory::WriteMBCControlRegisters(const u16 addr, const u8 data) {
    switch (mbc_mode) {
    case MBC::MBC1:
    case MBC::MBC1M:
        if (addr < 0x2000) {
            // RAM enable register -- RAM banking is enabled if a byte with lower nibble 0xA is written
            if (ext_ram_present && (data & 0x0F) == 0x0A) {
                ext_ram_enabled = true;
            } else {
                ext_ram_enabled = false;
            }
        } else if (addr < 0x4000) {
            // ROM bank register
            // In MBC1, only the lower 5 bits of the written value are considered. In MBC1M, it's only the lower 4.
            // Preserve the upper bits.
            if (mbc_mode == MBC::MBC1) {
                rom_bank_num = (rom_bank_num & 0x60) | (data & 0x1F);
            } else {
                rom_bank_num = (rom_bank_num & 0x30) | (data & 0x0F);
            }

            // 0x00, 0x20, 0x40, 0x60 all map to 0x01, 0x21, 0x41, 0x61 respectively.
            if (rom_bank_num == 0x00 || rom_bank_num == 0x20 || rom_bank_num == 0x40 || rom_bank_num == 0x60) {
                ++rom_bank_num;
            }
        } else if (addr < 0x6000) {
            // RAM bank register (or upper bits ROM bank)
            // Only the lower 2 bits of the written value are considered.
            upper_bits = data & 0x03;
            if (ram_bank_mode) {
                ram_bank_num = upper_bits;
            } else {
                // In a regular MBC1, the upper bits are only used on the ROM1 bank number if RAM banking is disabled.
                rom_bank_num = (rom_bank_num & 0x1F) | upper_bits << 5;
            }

            // In a MBC1M, the upper bits always affect the ROM1 bank number.
            if (mbc_mode == MBC::MBC1M) {
                rom_bank_num = (rom_bank_num & 0x0F) | upper_bits << 4;
            }
        } else if (addr < 0x8000) {
            // RAM banking mode -- selects whether the two bits in the above register are used as the RAM bank number
            // or the upper bits of the ROM1 bank number.
            // In MBC1M, enabling RAM banking also causes the upper bits to be used as the ROM0 bank offset. In
            // addition, the upper bits are always used as the upper bits of the ROM1 bank number, even when this
            // mode is enabled.
            if (ram_bank_mode != (data & 0x01)) {
                ram_bank_mode = data & 0x01;
                if (ram_bank_mode) {
                    // The upper bits are now used for the RAM bank number. In MBC1, they are no longer used for the
                    // upper bits of the ROM1 bank number.
                    ram_bank_num = upper_bits;
                    if (mbc_mode == MBC::MBC1) {
                        rom_bank_num &= 0x1F;
                    }
                } else {
                    // The upper bits are no longer used for the RAM bank number. In MBC1, they are now used for the
                    // upper bits of the ROM1 bank number.
                    ram_bank_num = 0x00;
                    if (mbc_mode == MBC::MBC1) {
                        rom_bank_num |= upper_bits << 5;
                    }
                }
            }
        }
        break;
    case MBC::MBC2:
        if (addr < 0x2000) {
            // RAM enable register -- RAM banking is enabled if a byte with lower nibble 0xA is written
            // The least significant bit of the upper address byte must be zero to enable or disable external ram.
            if (!(addr & 0x0100)) {
                if (ext_ram_present && (data & 0x0F) == 0x0A) {
                    ext_ram_enabled = true;
                } else {
                    ext_ram_enabled = false;
                }
            }
        } else if (addr < 0x4000) {
            // ROM bank register -- The least significant bit of the upper address byte must be 1 to switch ROM banks.
            if (addr & 0x0100) {
                // Only the lower 4 bits of the written value are considered.
                rom_bank_num = data & 0x0F;
                if (rom_bank_num == 0) {
                    ++rom_bank_num;
                }
            }
        }
        // MBC2 does not have RAM banking.
        break;
    case MBC::MBC3:
        if (addr < 0x2000) {
            // RAM banking and RTC registers enable register -- enabled if a byte with lower nibble 0xA is written.
            if (ext_ram_present && (data & 0x0F) == 0x0A) {
                ext_ram_enabled = true;
            } else {
                ext_ram_enabled = false;
            }
        } else if (addr < 0x4000) {
            // ROM bank register
            // The 7 lower bits of the written value select the ROM bank to be used at 0x4000-0x7FFF.
            rom_bank_num = data & 0x7F;

            // Selecting 0x00 will select bank 0x01. Unlike MBC1, the banks 0x20, 0x40, and 0x60 can all be selected.
            if (rom_bank_num == 0x00) {
                ++rom_bank_num;
            }
        } else if (addr < 0x6000) {
            // RAM bank selection or RTC register selection register
            // Values 0x00-0x07 select one of the RAM banks, and values 0x08-0x0C select one of the RTC registers.
            ram_bank_num = data & 0x0F;
        } else if (addr < 0x8000) {
            // Latch RTC data.
            // Writing a 0x00 then a 0x01 latches the current time into the RTC registers.
            if (rtc->latch_last_value_written == 0x00 && data == 0x01) {
                rtc->LatchCurrentTime();
            }

            rtc->latch_last_value_written = data;
        }
        break;
    case MBC::MBC5:
        if (addr < 0x2000) {
            // RAM banking enable register -- enabled if a byte with lower nibble 0xA is written.
            if (ext_ram_present && (data & 0x0F) == 0x0A) {
                ext_ram_enabled = true;
            } else {
                ext_ram_enabled = false;
            }
        } else if (addr < 0x3000) {
            // Low byte ROM bank register
            // This register selects the low 8 bits of the ROM bank to be used at 0x4000-0x7FFF.
            // Unlike both MBC1 and MBC3, ROM bank 0 can be mapped here.
            rom_bank_num = (rom_bank_num & 0xFF00) | data;
        } else if (addr < 0x4000) {
            // High byte ROM bank register
            // This register selects the high 8 bits of the ROM bank to be used at 0x4000-0x7FFF.
            // There is only one official game known to use more than 256 ROM banks (Densha de Go! 2), and it only 
            // uses bit 0 of this register.
            // If a game does not use more than 256 ROM banks, writes here are ignored.
            if (num_rom_banks > 256) {
                rom_bank_num = (rom_bank_num & 0x00FF) | (static_cast<unsigned int>(data) << 8);
            }
        } else if (addr < 0x6000) {
            // RAM bank selection.
            // Can have as many as 16 RAM banks. Carts with rumble activate it by writing 0x08 to this register, so
            // they cannot have more than 8 RAM banks.
            ram_bank_num = data & 0x0F;
        }
        break;

    default:
        // Carts with no MBC ignore writes here.
        break;
    }
}

} // End namespace Core
