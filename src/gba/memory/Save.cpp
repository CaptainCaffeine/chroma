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
        save_type = SaveType::SRam;
    } else if (save_size == 8 * kbyte || save_size == 512) {
        save_type = SaveType::Eeprom;
    } else if (save_size == 64 * kbyte || save_size == 128 * kbyte) {
        save_type = SaveType::Flash;
    } else if (save_size > 128 * kbyte) {
        throw std::runtime_error(fmt::format("Save game size of {} bytes is too large to be a GBA save.", save_size));
    } else {
        throw std::runtime_error(fmt::format("Invalid save game size: {} bytes.", save_size));
    }

    sram.resize(save_size);
    save_file.read(reinterpret_cast<char*>(sram.data()), save_size);
}

void Memory::WriteSaveFile() const {
    std::ofstream save_file(save_path);

    if (!save_file) {
        fmt::print("Error: could not open {} to write save file to disk.\n", save_path);
    } else {
        save_file.write(reinterpret_cast<const char*>(sram.data()), sram.size());
        save_file.flush();
    }
}

void Memory::InitSRam() {
    sram.resize(sram_size, 0xFF);
    save_type = SaveType::SRam;
}

} // End namespace Gba
