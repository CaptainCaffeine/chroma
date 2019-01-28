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
#include <unordered_map>
#include <fmt/format.h>

#include "gba/memory/Memory.h"
#include "gba/core/Core.h"
#include "gba/cpu/Disassembler.h"
#include "emu/ParseOptions.h"

namespace Gba {

void Memory::ReadSaveFile() {
    std::ifstream save_file(save_path);
    if (!save_file) {
        // Save file doesn't exist.
        CheckSaveOverrides();
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
            chip_id = FlashId::Sanyo;
        }
    } else if (save_size > 128 * kbyte) {
        throw std::runtime_error(fmt::format("Save game size of {} bytes is too large to be a GBA save.", save_size));
    } else {
        throw std::runtime_error(fmt::format("Invalid save game size: {} bytes.", save_size));
    }
}

void Memory::WriteSaveFile() const {
    if (save_type == SaveType::Unknown || save_type == SaveType::None) {
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

static constexpr u64 ByteSwap64(u64 value) noexcept {
    u64 swapped_value =   ((value & 0x0000'0000'0000'00FF) << 56)
                        | ((value & 0x0000'0000'0000'FF00) << 40)
                        | ((value & 0x0000'0000'00FF'0000) << 24)
                        | ((value & 0x0000'0000'FF00'0000) <<  8)
                        | ((value & 0x0000'00FF'0000'0000) >>  8)
                        | ((value & 0x0000'FF00'0000'0000) >> 24)
                        | ((value & 0x00FF'0000'0000'0000) >> 40)
                        | ((value & 0xFF00'0000'0000'0000) >> 56);

    return swapped_value;
}

void Memory::ParseEepromCommand() {
    if (save_type != SaveType::Eeprom) {
        return;
    }

    const auto stream_size = eeprom_bitstream.size();
    if (!eeprom_ready || stream_size < 9) {
        if (!eeprom_ready) {
            core.disasm->LogAlways("ParseEepromCommand when eeprom not ready\n");
        } else {
            core.disasm->LogAlways("ParseEepromCommand when stream size too small: {}\n", stream_size);
        }
        eeprom_bitstream.clear();
        return;
    }

    if (eeprom_bitstream[0] != 1) {
        // Malformed request type.
        core.disasm->LogAlways("First bit of bitstream not 1.\n");
        eeprom_bitstream.clear();
        return;
    }

    const bool read_request = eeprom_bitstream[1] == 1;
    const u16 eeprom_addr = ParseEepromAddr(stream_size, read_request);
    if (eeprom_addr == 0xFFFF) {
        eeprom_bitstream.clear();
        return;
    }

    if (read_request) {
        if (eeprom_addr <= 0x3FF) {
            eeprom_read_buffer = ByteSwap64(eeprom[eeprom_addr]);
        } else {
            // OOB EEPROM reads return all 1s.
            eeprom_read_buffer = -1;
        }
        eeprom_read_pos = 0;
    } else if (eeprom_addr <= 0x3FF) {
        // OOB EEPROM writes are ignored.
        u64 value = 0;
        for (int i = 0; i < 64; ++i) {
            // Values are written MSBit first.
            value |= static_cast<u64>(eeprom_bitstream[i + 2 + eeprom_addr_len]) << (63 - i);
        }

        // We store the EEPROM data as big-endian for compatibility with mGBA.
        eeprom[eeprom_addr] = ByteSwap64(value);
        eeprom_ready = 0;
        delayed_op = {eeprom_write_cycles, [this]() {
            eeprom_ready = 1;
        }};
    }

    eeprom_bitstream.clear();
}

u16 Memory::ParseEepromAddr(int stream_size, bool read_request) {
    const int non_addr_bits = read_request ? 3 : 67;

    if (eeprom_addr_len == 0) {
        InitEeprom(stream_size, non_addr_bits);
    }

    if (stream_size != non_addr_bits + eeprom_addr_len) {
        // Invalid size.
        core.disasm->LogAlways("Invalid bitstream size: {}.\n", stream_size);
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
        if (data == FlashCmd::Start1 && addr == FlashAddr::Command1) {
            // Start a new command.
            flash_state = FlashState::Starting;
        }
        break;

    case FlashState::Starting:
        if (data == FlashCmd::Start2 && addr == FlashAddr::Command2) {
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
        } else if (addr == FlashAddr::Command1) {
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

void Memory::CheckSaveOverrides() {
    // Credit goes to mGBA for this list of save overrides. As of June 2018, the original can be found here:
    // https://github.com/mgba-emu/mgba/blob/master/src/gba/overrides.c
    const std::unordered_map<std::string, SaveType> save_overrides {
        // Advance Wars
        {"AWRE", SaveType::Flash},
        {"AWRP", SaveType::Flash},

        // Advance Wars 2: Black Hole Rising
        {"AW2E", SaveType::Flash},
        {"AW2P", SaveType::Flash},

        // Boktai: The Sun is in Your Hand
        {"U3IJ", SaveType::Eeprom},
        {"U3IE", SaveType::Eeprom},
        {"U3IP", SaveType::Eeprom},

        // Boktai 2: Solar Boy Django
        {"U32J", SaveType::Eeprom},
        {"U32E", SaveType::Eeprom},
        {"U32P", SaveType::Eeprom},

        // Crash Bandicoot 2 - N-Tranced
        {"AC8J", SaveType::Eeprom},
        {"AC8E", SaveType::Eeprom},
        {"AC8P", SaveType::Eeprom},

        // Dragon Ball Z - The Legacy of Goku
        {"ALGP", SaveType::Eeprom},

        // Dragon Ball Z - The Legacy of Goku II
        {"ALFJ", SaveType::Eeprom},
        {"ALFE", SaveType::Eeprom},
        {"ALFP", SaveType::Eeprom},

        // Dragon Ball Z - Taiketsu
        {"BDBE", SaveType::Eeprom},
        {"BDBP", SaveType::Eeprom},

        // Drill Dozer
        {"V49J", SaveType::SRam},
        {"V49E", SaveType::SRam},

        // Final Fantasy Tactics Advance
        {"AFXE", SaveType::Flash},

        // F-Zero - Climax
        {"BFTJ", SaveType::Flash128},

        // Iridion II
        {"AI2E", SaveType::None},
        {"AI2P", SaveType::None},

        // Golden Sun: The Lost Age
        {"AGFE", SaveType::Flash},

        // Koro Koro Puzzle - Happy Panechu!
        {"KHPJ", SaveType::Eeprom},

        // Mega Man Battle Network
        {"AREE", SaveType::SRam},

        // Mega Man Zero
        {"AZCE", SaveType::SRam},

        // Metal Slug Advance
        {"BSME", SaveType::Eeprom},

        // Pokemon Pinball: Ruby & Sapphire
        {"BPPJ", SaveType::SRam},
        {"BPPE", SaveType::SRam},
        {"BPPP", SaveType::SRam},
        {"BPPU", SaveType::SRam},

        // Pokemon Ruby
        {"AXVJ", SaveType::Flash128},
        {"AXVE", SaveType::Flash128},
        {"AXVP", SaveType::Flash128},
        {"AXVI", SaveType::Flash128},
        {"AXVS", SaveType::Flash128},
        {"AXVD", SaveType::Flash128},
        {"AXVF", SaveType::Flash128},

        // Pokemon Sapphire
        {"AXPJ", SaveType::Flash128},
        {"AXPE", SaveType::Flash128},
        {"AXPP", SaveType::Flash128},
        {"AXPI", SaveType::Flash128},
        {"AXPS", SaveType::Flash128},
        {"AXPD", SaveType::Flash128},
        {"AXPF", SaveType::Flash128},

        // Pokemon Emerald
        {"BPEJ", SaveType::Flash128},
        {"BPEE", SaveType::Flash128},
        {"BPEP", SaveType::Flash128},
        {"BPEI", SaveType::Flash128},
        {"BPES", SaveType::Flash128},
        {"BPED", SaveType::Flash128},
        {"BPEF", SaveType::Flash128},

        // Pokemon Mystery Dungeon
        {"B24J", SaveType::Flash128},
        {"B24E", SaveType::Flash128},
        {"B24P", SaveType::Flash128},
        {"B24U", SaveType::Flash128},

        // Pokemon FireRed
        {"BPRJ", SaveType::Flash128},
        {"BPRE", SaveType::Flash128},
        {"BPRP", SaveType::Flash128},
        {"BPRI", SaveType::Flash128},
        {"BPRS", SaveType::Flash128},
        {"BPRD", SaveType::Flash128},
        {"BPRF", SaveType::Flash128},

        // Pokemon LeafGreen
        {"BPGJ", SaveType::Flash128},
        {"BPGE", SaveType::Flash128},
        {"BPGP", SaveType::Flash128},
        {"BPGI", SaveType::Flash128},
        {"BPGS", SaveType::Flash128},
        {"BPGD", SaveType::Flash128},
        {"BPGF", SaveType::Flash128},

        // RockMan EXE 4.5 - Real Operation
        {"BR4J", SaveType::Flash},

        // Rocky
        {"AR8E", SaveType::Eeprom},
        {"AROP", SaveType::Eeprom},

        // Sennen Kazoku
        {"BKAJ", SaveType::Flash128},

        // Shin Bokura no Taiyou: Gyakushuu no Sabata
        {"U33J", SaveType::Eeprom},

        // Super Mario Advance 2
        {"AA2J", SaveType::Eeprom},
        {"AA2E", SaveType::Eeprom},

        // Super Mario Advance 3
        {"A3AJ", SaveType::Eeprom},
        {"A3AE", SaveType::Eeprom},
        {"A3AP", SaveType::Eeprom},

        // Super Mario Advance 4
        {"AX4J", SaveType::Flash128},
        {"AX4E", SaveType::Flash128},
        {"AX4P", SaveType::Flash128},

        // Super Monkey Ball Jr.
        {"ALUE", SaveType::Eeprom},
        {"ALUP", SaveType::Eeprom},

        // Top Gun - Combat Zones
        {"A2YE", SaveType::None},

        // Ueki no Housoku - Jingi Sakuretsu! Nouryokusha Battle
        {"BUHJ", SaveType::Eeprom},

        // Wario Ware Twisted
        {"RZWJ", SaveType::SRam},
        {"RZWE", SaveType::SRam},
        {"RZWP", SaveType::SRam},

        // Yoshi's Universal Gravitation
        {"KYGJ", SaveType::Eeprom},
        {"KYGE", SaveType::Eeprom},
        {"KYGP", SaveType::Eeprom},

        // Aging cartridge
        {"TCHK", SaveType::Eeprom},
    };

    // Read the game code from the ROM header and see if it's in our list of overrides.
    const std::string game_code{reinterpret_cast<const char*>(rom.data()) + 0xAC, 4};

    auto game_override = save_overrides.find(game_code);
    if (game_override != save_overrides.end()) {
        switch (game_override->second) {
        case SaveType::SRam:
            fmt::print("SRAM override\n");

            save_type = SaveType::SRam;
            sram.resize(sram_size);
            sram_addr_mask = sram_size - 1;
            break;
        case SaveType::Eeprom:
            fmt::print("EEPROM override\n");

            save_type = SaveType::Eeprom;
            break;
        case SaveType::Flash:
            fmt::print("64KB Flash override\n");

            save_type = SaveType::Flash;
            sram.resize(flash_size, 0xFF);
            sram_addr_mask = flash_size - 1;
            break;
        case SaveType::Flash128:
            fmt::print("128KB Flash override\n");

            save_type = SaveType::Flash;
            sram.resize(flash_size * 2, 0xFF);
            sram_addr_mask = flash_size - 1;
            chip_id = FlashId::Sanyo;
            break;
        case SaveType::None:
            fmt::print("Force no save game\n");

            save_type = SaveType::None;
            break;
        default:
            break;
        }
    } else if (game_code[0] == 'F') {
        // Classic NES Series override.
        fmt::print("EEPROM override\n");
        save_type = SaveType::Eeprom;
    } else {
        save_type = SaveType::Unknown;
    }
}

void Memory::CheckHardwareOverrides() {
    // Read the game code from the ROM header.
    const std::string game_code{reinterpret_cast<const char*>(rom.data()) + 0xAC, 4};

    if (game_code[0] == 'F') {
        // In Classic NES Series games, the ROM contents are mirrored throughout the ROM region.
        rom_addr_mask = rom_size - 1;
    } else {
        rom_addr_mask = rom_max_size - 1;
    }
}

} // End namespace Gba
