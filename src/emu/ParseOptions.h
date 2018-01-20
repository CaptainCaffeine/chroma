// This file is a part of Chroma.
// Copyright (C) 2016-2017 Matthew Murray
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

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"
#include "gb/core/Enums.h"

namespace Gb { class CartridgeHeader; }

namespace Emu {

std::vector<std::string> GetTokens(char** begin, char** end);
bool ContainsOption(const std::vector<std::string>& tokens, const std::string& option);
std::string GetOptionParam(const std::vector<std::string>& tokens, const std::string& option);

void DisplayHelp();
Gb::Console GetGameBoyType(const std::vector<std::string>& tokens);
LogLevel GetLogLevel(const std::vector<std::string>& tokens);
unsigned int GetPixelScale(const std::vector<std::string>& tokens);
bool GetFilterEnable(const std::vector<std::string>& tokens);

std::streampos GetFileSize(std::ifstream& filestream);
Gb::Console CheckRomFile(const std::string& filename);
std::string SaveGamePath(const std::string& rom_path);
std::vector<u8> LoadSaveGame(const Gb::CartridgeHeader& cart_header, const std::string& save_path);
std::vector<u8> ReadSaveFile(const std::string& filename);
std::vector<u32> LoadGbaBios();
void CheckPathIsRegularFile(const std::string& filename);

template<typename T>
std::vector<T> LoadRom(const std::string& filename) {
    std::ifstream rom_file(filename);
    if (!rom_file) {
        throw std::runtime_error("Error when attempting to open " + filename);
    }

    const auto rom_size = GetFileSize(rom_file);

    std::vector<T> rom_contents(rom_size / sizeof(T));
    rom_file.read(reinterpret_cast<char*>(rom_contents.data()), rom_size);

    return rom_contents;
}

} // End namespace Emu
