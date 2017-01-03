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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sys/stat.h>

#include "ParseOptions.h"

namespace Emu {

std::vector<std::string> GetTokens(char** begin, char** end) {
    std::vector<std::string> tokens;
    for (; begin != end; ++begin) {
        tokens.emplace_back(*begin);
    }

    return tokens;
}

bool ContainsOption(const std::vector<std::string>& tokens, const std::string& option) {
    return std::find(tokens.cbegin(), tokens.cend(), option) != tokens.cend();
}

std::string GetOptionParam(const std::vector<std::string>& tokens, const std::string& option) {
    auto itr = std::find(tokens.cbegin(), tokens.cend(), option);
    if (itr != tokens.cend() && ++itr != tokens.cend()) {
        return *itr;
    }
    return std::string();
}

void DisplayHelp() {
    std::cout << "Usage: chroma [options] <path/to/rom>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h\t\t\t\tdisplay help\n";
    std::cout << "  -m [dmg, cgb]\t\t\tspecify device to emulate (default: dmg)\n";
    std::cout << "  -l [regular, timer, lcd]\tspecify log level (default: none)" << std::endl;
}

Console GetGameBoyType(const std::vector<std::string>& tokens) {
    const std::string gb_string = Emu::GetOptionParam(tokens, "-m");
    if (!gb_string.empty()) {
        if (gb_string == "dmg") {
            return Console::DMG;
        } else if (gb_string == "cgb") {
            return Console::CGB;
        } else {
            throw std::invalid_argument("Invalid console specified: " + gb_string);
        }
    } else {
        // If no console specified, default is DMG.
        return Console::DMG;
    }
}

LogLevel GetLogLevel(const std::vector<std::string>& tokens) {
    const std::string log_string = Emu::GetOptionParam(tokens, "-l");
    if (!log_string.empty()) {
        if (log_string == "regular") {
            return LogLevel::Regular;
        } else if (log_string == "timer") {
            return LogLevel::Timer;
        } else if (log_string == "lcd") {
            return LogLevel::LCD;
        } else {
            throw std::invalid_argument("Invalid log level specified: " + log_string);
        }
    } else {
        // If no log level specified, then no logging by default.
        return LogLevel::None;
    }
}

std::vector<u8> LoadROM(const std::string& filename) {
    std::ifstream rom_file(filename);
    if (!rom_file) {
        throw std::invalid_argument("Error when attempting to open " + filename);
    }

    // Check that the path points to a regular file.
    struct stat stat_info;
    if (stat(filename.c_str(), &stat_info) == 0) {
        if (stat_info.st_mode & S_IFDIR) {
            throw std::invalid_argument("Provided path is a directory: " + filename);
        } else if (!(stat_info.st_mode & S_IFREG)) {
            throw std::invalid_argument("Provided path is not a regular file: " + filename);
        }
    }

    rom_file.seekg(0, std::ios_base::end);
    const auto rom_size = rom_file.tellg();
    rom_file.seekg(0, std::ios_base::beg);

    if (rom_size < 0x8000) {
        throw std::invalid_argument("Rom size of " + std::to_string(rom_size)
                                    + " bytes is too small to be a Game Boy game.");
    } else if (rom_size > 0x800000) {
        throw std::invalid_argument("Rom size of " + std::to_string(rom_size)
                                    + " bytes is too large to be a Game Boy game.");
    }

    std::vector<u8> rom_contents(rom_size);
    rom_file.read(reinterpret_cast<char*>(rom_contents.data()), rom_size);

    return rom_contents;
}

std::ofstream OpenLogFile(const std::string& rom_path) {
    // Get path of directory containing the ROM, then add log file name.
    auto last_slash = rom_path.find_last_of("/\\");
    const std::string log_file_path = rom_path.substr(0, last_slash + 1) + "log.txt";

    std::ofstream log_file(log_file_path);
    if (!log_file) {
        throw std::invalid_argument("Error when attempting to open " + log_file_path + " for writing.");
    }

    return log_file;
}

} // End namespace Emu
