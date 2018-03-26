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
        save_type = SaveType::Unknown;
        return;
    }

    Emu::CheckPathIsRegularFile(save_path);

    const auto save_size = Emu::GetFileSize(save_file);

    if (save_size == 32 * kbyte) {
        fmt::print("Found SRAM save\n");

        save_type = SaveType::SRam;
        sram.resize(save_size);
        save_file.read(reinterpret_cast<char*>(sram.data()), save_size);
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
    } else if (save_size > 128 * kbyte) {
        throw std::runtime_error(fmt::format("Save game size of {} bytes is too large to be a GBA save.", save_size));
    } else {
        throw std::runtime_error(fmt::format("Invalid save game size: {} bytes.", save_size));
    }
}

void Memory::WriteSaveFile() const {
    if (save_type != SaveType::SRam && save_type != SaveType::Eeprom) {
        return;
    }

    std::ofstream save_file(save_path);

    if (!save_file) {
        fmt::print("Error: could not open {} to write save file to disk.\n", save_path);
    } else {
        if (save_type == SaveType::SRam) {
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
}

void Memory::EepromWrite(int cycles) {
    if (eeprom_write_cycles > 0) {
        eeprom_write_cycles -= cycles;
        if (eeprom_write_cycles <= 0) {
            eeprom_ready = 1;
        }
    }
}

void Memory::ParseEepromCommand() {
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
        eeprom_write_cycles = 108368;
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

} // End namespace Gba
