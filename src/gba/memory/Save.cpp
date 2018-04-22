// This file is a part of Chroma.
// Copyright (C) 2018 Matthew Murray
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

#include <stdexcept>
#include <fstream>
#include <fmt/format.h>

#include "gba/memory/Memory.h"
#include "emu/ParseOptions.h"

namespace Gba {

void Memory::ReadSaveFile() {
    std::ifstream save_file(save_path);
    if (!save_file) {
        // Save file doesn't exist.

        // Read the game code from the ROM header and see if it's in our list of overrides.
        std::string game_code{reinterpret_cast<const char*>(rom.data()) + 0xAC, 4};
        if (game_code == "AXVE" || game_code == "BPEE" || game_code == "BPRE"
                                || game_code == "B24E" || game_code == "AX4E") {
            fmt::print("128KB Flash override\n");
            sram.resize(flash_size * 2, 0xFF);
            save_type = SaveType::Flash;

            sram_addr_mask = flash_size - 1;
            chip_id = sanyo_id;
        } else {
            save_type = SaveType::Unknown;
        }

        return;
    }

    Emu::CheckPathIsRegularFile(save_path);

    const auto save_size = Emu::GetFileSize(save_file);

    if (save_size == 32 * kbyte) {
        fmt::print("Found SRAM save\n");

        save_type = SaveType::SRam;
        sram.resize(save_size);
        save_file.read(reinterpret_cast<char*>(sram.data()), save_size);
        sram_addr_mask = sram_size - 1;
    } else if (save_size == 8 * kbyte || save_size == 512) {
        fmt::print("Found EEPROM save\n");

        save_type = SaveType::Eeprom;
        eeprom.resize(save_size / sizeof(u64));
        save_file.read(reinterpret_cast<char*>(eeprom.data()), save_size);

        if (save_size == 8 * kbyte) {
            eeprom_addr_len = 14;
        } else {
            eeprom_addr_len = 6;
        }
    } else if (save_size == 64 * kbyte || save_size == 128 * kbyte) {
        fmt::print("Found Flash save\n");

        save_type = SaveType::Flash;
        sram.resize(save_size);
        save_file.read(reinterpret_cast<char*>(sram.data()), save_size);
        sram_addr_mask = flash_size - 1;

        if (save_size == flash_size * 2) {
            // 128KB flash.
            chip_id = sanyo_id;
        }
    } else if (save_size > 128 * kbyte) {
        throw std::runtime_error(fmt::format("Save game size of {} bytes is too large to be a GBA save.", save_size));
    } else {
        throw std::runtime_error(fmt::format("Invalid save game size: {} bytes.", save_size));
    }
}

void Memory::WriteSaveFile() const {
    if (save_type == SaveType::Unknown) {
        return;
    }

    std::ofstream save_file(save_path);

    if (!save_file) {
        fmt::print("Error: could not open {} to write save file to disk.\n", save_path);
    } else {
        if (save_type == SaveType::SRam || save_type == SaveType::Flash) {
            save_file.write(reinterpret_cast<const char*>(sram.data()), sram.size());
        } else if (save_type == SaveType::Eeprom) {
            save_file.write(reinterpret_cast<const char*>(eeprom.data()), eeprom.size() * sizeof(u64));
        }
        save_file.flush();
    }
}

void Memory::InitSRam() {
    fmt::print("SRAM detected\n");
    sram.resize(sram_size, 0xFF);
    save_type = SaveType::SRam;

    sram_addr_mask = sram_size - 1;
}

void Memory::InitFlash() {
    fmt::print("Flash detected\n");
    sram.resize(flash_size, 0xFF);
    save_type = SaveType::Flash;

    sram_addr_mask = flash_size - 1;
}

void Memory::DelayedSaveOp(int cycles) {
    if (delayed_op.cycles > 0) {
        delayed_op.cycles -= cycles;
        if (delayed_op.cycles <= 0) {
            delayed_op.action();
        }
    }
}

void Memory::ParseEepromCommand() {
    if (save_type != SaveType::Eeprom) {
        return;
    }

    const auto stream_size = eeprom_bitstream.size();
    if (!eeprom_ready || stream_size < 9) {
        if (!eeprom_ready) {
            fmt::print("ParseEepromCommand when eeprom not ready\n");
        } else {
            fmt::print("ParseEepromCommand when stream size too small: {}\n", stream_size);
        }
        eeprom_bitstream.clear();
        return;
    }

    if (eeprom_bitstream[0] != 1) {
        // Malformed request type.
        fmt::print("First bit of bitstream not 1.\n");
        eeprom_bitstream.clear();
        return;
    }

    const bool read_request = eeprom_bitstream[1] == 1;
    u16 eeprom_addr = ParseEepromAddr(stream_size, read_request ? 3 : 67);
    if (eeprom_addr == 0xFFFF) {
        eeprom_bitstream.clear();
        return;
    }

    if (read_request) {
        if (eeprom_addr <= 0x3FF) {
            eeprom_read_buffer = eeprom[eeprom_addr];
        } else {
            // OOB EEPROM reads return all 1s.
            eeprom_read_buffer = -1;
        }
        eeprom_read_pos = 0;
    } else if (eeprom_addr <= 0x3FF) {
        // OOB EEPROM writes are ignored.
        u64 value = 0;
        for (int i = 0; i < 64; ++i) {
            value |= static_cast<u64>(eeprom_bitstream[i + 2 + eeprom_addr_len]) << i;
        }

        eeprom[eeprom_addr] = value;
        eeprom_ready = 0;
        delayed_op = {108368, [this]() {
            eeprom_ready = 1;
        }};
    }

    eeprom_bitstream.clear();
}

u16 Memory::ParseEepromAddr(int stream_size, int non_addr_bits) {
    if (eeprom_addr_len == 0) {
        InitEeprom(stream_size, non_addr_bits);
    }

    if (stream_size != non_addr_bits + eeprom_addr_len) {
        // Invalid size.
        fmt::print("Invalid bitstream size: {}.\n", stream_size);
        eeprom_bitstream.clear();
        return 0xFFFF;
    }

    u16 eeprom_addr = 0;
    for (int i = 0; i < eeprom_addr_len; ++i) {
        // The EEPROM address is written MSB first.
        eeprom_addr |= static_cast<u16>(eeprom_bitstream[i + 2]) << (eeprom_addr_len - 1 - i);
    }

    return eeprom_addr;
}

void Memory::InitEeprom(int stream_size, int non_addr_bits) {
    if (stream_size == non_addr_bits + 6) {
        fmt::print("EEPROM addr length detected as 6.\n");
        eeprom_addr_len = 6;
        eeprom.resize(0x40);
    } else if (stream_size == non_addr_bits + 14) {
        fmt::print("EEPROM addr length detected as 14.\n");
        eeprom_addr_len = 14;
        eeprom.resize(0x400);
    } else {
        // Hopefully the next stream will be well-sized.
        fmt::print("Could not determine EEPROM addr length: {}.\n", stream_size);
        return;
    }
}

template <typename T>
void Memory::WriteFlash(const u32 addr, const T data) {
    switch (flash_state) {
    case FlashState::Command:
        if (last_flash_cmd == FlashCmd::Write) {
            delayed_op = {flash_write_cycles, [this, addr, data]() {
                WriteSRam(addr, data);
            }};
        } else if (last_flash_cmd == FlashCmd::BankSwitch) {
            if (sram.size() == flash_size * 2) {
                bank_num = data & 0x1;
            }
        }

        flash_state = FlashState::NotStarted;
        last_flash_cmd = FlashCmd::None;
        break;

    case FlashState::NotStarted:
        if (data == FlashCmd::Start1 && addr == flash_cmd_addr1) {
            // Start a new command.
            flash_state = FlashState::Starting;
        }
        break;

    case FlashState::Starting:
        if (data == FlashCmd::Start2 && addr == flash_cmd_addr2) {
            flash_state = FlashState::Ready;
        } else {
            // Does it actually reset the state machine here if we receive something other than Start2? Or does
            // it just stay in the starting state?
            flash_state = FlashState::NotStarted;
        }
        break;

    case FlashState::Ready:
        if (last_flash_cmd == FlashCmd::Erase && data == FlashCmd::EraseSector) {
            delayed_op = {flash_erase_cycles, [this, addr]() {
                std::fill_n(sram.begin() + bank_num * flash_size + (addr & 0x0000'F000), 0x1000, 0xFF);
            }};

            flash_state = FlashState::NotStarted;
        } else if (addr == flash_cmd_addr1) {
            flash_state = FlashState::NotStarted;

            switch (data) {
            case EnterIdMode:
                flash_id_mode = true;
                break;
            case ExitIdmode:
                flash_id_mode = false;
                break;
            case Erase:
                break;
            case EraseChip:
                if (last_flash_cmd == FlashCmd::Erase) {
                    delayed_op = {flash_erase_cycles, [this]() {
                        std::fill(sram.begin(), sram.end(), 0xFF);
                    }};
                }
                break;
            case EraseSector:
                break;
            case Write:
                flash_state = FlashState::Command;
                break;
            case BankSwitch:
                flash_state = FlashState::Command;
                break;
            default:
                break;
            }
        }

        last_flash_cmd = static_cast<FlashCmd>(data);
        break;
    default:
        break;
    }
}

template void Memory::WriteFlash<u8>(const u32 addr, const u8 data);
template void Memory::WriteFlash<u16>(const u32 addr, const u16 data);
template void Memory::WriteFlash<u32>(const u32 addr, const u32 data);

} // End namespace Gba
