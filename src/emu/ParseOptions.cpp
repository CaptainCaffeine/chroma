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

#include <algorithm>
#include <array>
#include <iostream>
#include <sys/stat.h>

#include "gb/memory/CartridgeHeader.h"
#include "emu/ParseOptions.h"

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

    return "";
}

void DisplayHelp() {
    std::cout << "Usage: chroma [options] <path/to/rom>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h                       display help\n";
    std::cout << "  -m [dmg, cgb, agb]       specify device to emulate\n";
    std::cout << "  -l [regular, timer, lcd] specify log level (default: none)\n";
    std::cout << "  -s [1-15]                specify resolution scale (default: 2)\n";
    std::cout << "  -f                       activate fullscreen mode\n";
    std::cout << "  --filter [iir, nearest]  choose audio filtering method (default: iir)\n";
    std::cout << "                               IIR (slow, better quality)\n";
    std::cout << "                               nearest-neighbour (fast, lesser quality)\n";
    std::cout << "  --multicart              emulate this game using an MBC1M" << std::endl;
}

Console GetGameBoyType(const std::vector<std::string>& tokens) {
    const std::string gb_string = Emu::GetOptionParam(tokens, "-m");
    if (!gb_string.empty()) {
        if (gb_string == "dmg") {
            return Console::DMG;
        } else if (gb_string == "cgb") {
            return Console::CGB;
        } else if (gb_string == "agb") {
            return Console::AGB;
        } else {
            throw std::invalid_argument("Invalid console specified: " + gb_string);
        }
    } else {
        // If no console specified, the console type will default to the cart type.
        return Console::Default;
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

unsigned int GetPixelScale(const std::vector<std::string>& tokens) {
    const std::string scale_string = Emu::GetOptionParam(tokens, "-s");
    if (!scale_string.empty()) {
        unsigned int scale = std::stoi(scale_string);
        if (scale > 15) {
            throw std::invalid_argument("Invalid scale value specified: " + scale_string);
        }

        return scale;
    } else {
        // If no resolution scale specified, default to 2x native resolution.
        return 2;
    }
}

bool GetFilterEnable(const std::vector<std::string>& tokens) {
    const std::string filter_string = Emu::GetOptionParam(tokens, "--filter");
    if (!filter_string.empty()) {
        if (filter_string == "iir") {
            return true;
        } else if (filter_string == "nearest") {
            return false;
        } else {
            throw std::invalid_argument("Invalid filter method specified: " + filter_string);
        }
    } else {
        // If no filter specified, default to using IIR filter.
        return true;
    }
}

static bool CheckAgbNintendoLogo(const std::vector<u8>& rom_header) {
    static constexpr std::array<u8, 156> logo{{
        0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21, 0x3D, 0x84, 0x82, 0x0A, 0x84,
        0xE4, 0x09, 0xAD, 0x11, 0x24, 0x8B, 0x98, 0xC0, 0x81, 0x7F, 0x21, 0xA3, 0x52,
        0xBE, 0x19, 0x93, 0x09, 0xCE, 0x20, 0x10, 0x46, 0x4A, 0x4A, 0xF8, 0x27, 0x31,
        0xEC, 0x58, 0xC7, 0xE8, 0x33, 0x82, 0xE3, 0xCE, 0xBF, 0x85, 0xF4, 0xDF, 0x94,
        0xCE, 0x4B, 0x09, 0xC1, 0x94, 0x56, 0x8A, 0xC0, 0x13, 0x72, 0xA7, 0xFC, 0x9F,
        0x84, 0x4D, 0x73, 0xA3, 0xCA, 0x9A, 0x61, 0x58, 0x97, 0xA3, 0x27, 0xFC, 0x03,
        0x98, 0x76, 0x23, 0x1D, 0xC7, 0x61, 0x03, 0x04, 0xAE, 0x56, 0xBF, 0x38, 0x84,
        0x00, 0x40, 0xA7, 0x0E, 0xFD, 0xFF, 0x52, 0xFE, 0x03, 0x6F, 0x95, 0x30, 0xF1,
        0x97, 0xFB, 0xC0, 0x85, 0x60, 0xD6, 0x80, 0x25, 0xA9, 0x63, 0xBE, 0x03, 0x01,
        0x4E, 0x38, 0xE2, 0xF9, 0xA2, 0x34, 0xFF, 0xBB, 0x3E, 0x03, 0x44, 0x78, 0x00,
        0x90, 0xCB, 0x88, 0x11, 0x3A, 0x94, 0x65, 0xC0, 0x7C, 0x63, 0x87, 0xF0, 0x3C,
        0xAF, 0xD6, 0x25, 0xE4, 0x8B, 0x38, 0x0A, 0xAC, 0x72, 0x21, 0xD4, 0xF8, 0x07
    }};

    auto logo_iter = logo.cbegin();
    for (std::size_t addr = 4; addr < logo.size() + 4; ++addr) {
        if (rom_header[addr] != *logo_iter++) {
            return false;
        }
    }

    return true;
}

Console CheckRomFile(const std::string& filename) {
    std::ifstream rom_file(filename);
    if (!rom_file) {
        throw std::runtime_error("Error when attempting to open " + filename);
    }

    CheckPathIsRegularFile(filename);

    rom_file.seekg(0, std::ios_base::end);
    const auto rom_size = rom_file.tellg();
    rom_file.seekg(0, std::ios_base::beg);

    if (rom_size < 0x8000) {
        // 32KB is the smallest possible GB game.
        throw std::runtime_error("Rom size of " + std::to_string(rom_size)
                                 + " bytes is too small to be a GB or GBA game.");
    } else if (rom_size > 0x2000000) {
        // 32MB is the largest possible GBA game.
        throw std::runtime_error("Rom size of " + std::to_string(rom_size)
                                 + " bytes is too large to be a GB or GBA game.");
    }

    // Read the first 0x134 bytes to check for the Nintendo logos.
    std::vector<u8> rom_header(0x134);
    rom_file.read(reinterpret_cast<char*>(rom_header.data()), rom_header.size());

    if (CheckAgbNintendoLogo(rom_header)) {
        return Console::AGB;
    } else if (Gb::CartridgeHeader::CheckNintendoLogo(Console::CGB, rom_header)) {
        return Console::CGB;
    } else {
        throw std::runtime_error("Provided ROM is neither a GB or GBA game. No valid Nintendo logo found.");
    }

    return Console::CGB;
}

std::string SaveGamePath(const std::string& rom_path) {
    std::size_t last_dot = rom_path.rfind('.');

    if (last_dot == std::string::npos) {
        throw std::runtime_error("No file extension found.");
    }

    if (rom_path.substr(last_dot, rom_path.size()) == ".sav") {
        throw std::runtime_error("You tried to run a save file instead of a ROM.");
    }

    return rom_path.substr(0, last_dot) + ".sav";
}

std::vector<u8> LoadSaveGame(const Gb::CartridgeHeader& cart_header, const std::string& save_path) {
    std::vector<u8> save_game;
    if (cart_header.ext_ram_present) {
        save_game = Emu::ReadSaveFile(save_path);

        if (save_game.size() != 0) {
            unsigned int cart_ram_size = cart_header.ram_size;
            if (cart_header.rtc_present) {
                // Account for size of RTC save data, if present at the end of the save file.
                if (save_game.size() % 0x400 == 0x30) {
                    cart_ram_size += 0x30;
                }
            }

            if (cart_ram_size != save_game.size()) {
                throw std::runtime_error("Save game size does not match external RAM size given in cartridge header.");
            }
        } else {
            // No preexisting save game.
            save_game = std::vector<u8>(cart_header.ram_size);
        }
    }

    return save_game;
}

std::vector<u8> ReadSaveFile(const std::string& filename) {
    CheckPathIsRegularFile(filename);

    std::ifstream save_file(filename);
    if (!save_file) {
        // Save file doesn't exist.
        return std::vector<u8>();
    }

    save_file.seekg(0, std::ios_base::end);
    const auto save_size = save_file.tellg();
    save_file.seekg(0, std::ios_base::beg);

    if (save_size > 0x20030) {
        throw std::runtime_error("Save game size of " + std::to_string(save_size)
                                 + " bytes is too large to be a Game Boy save.");
    }

    std::vector<u8> save_contents(save_size);
    save_file.read(reinterpret_cast<char*>(save_contents.data()), save_size);

    return save_contents;
}

void CheckPathIsRegularFile(const std::string& filename) {
    // Check that the path points to a regular file.
    struct stat stat_info;
    if (stat(filename.c_str(), &stat_info) == 0) {
        if (stat_info.st_mode & S_IFDIR) {
            throw std::runtime_error("Provided path is a directory: " + filename);
        } else if (!(stat_info.st_mode & S_IFREG)) {
            throw std::runtime_error("Provided path is not a regular file: " + filename);
        }
    }
}

} // End namespace Emu
