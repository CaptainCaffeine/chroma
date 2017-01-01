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

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"
#include "core/CartridgeHeader.h"
#include "core/Logging.h"
#include "core/GameBoy.h"
#include "emu/ParseOptions.h"
#include "emu/SDL_Utils.h"

int main(int argc, char** argv) {
    std::vector<std::string> tokens = Emu::GetTokens(argv, argv+argc);

    if (tokens.size() == 1 || Emu::ContainsOption(tokens, "-h")) {
        Emu::DisplayHelp();
        return 1;
    }

    Console gameboy_type;
    std::string gb_string = Emu::GetOptionParam(tokens, "-m");
    if (!gb_string.empty()) {
        if (gb_string == "dmg") {
            gameboy_type = Console::DMG;
        } else if (gb_string == "cgb") {
            gameboy_type = Console::CGB;
        } else {
            std::cerr << "Invalid console specified: " << gb_string << ". Valid consoles are dmg and cgb." << std::endl;
            return 1;
        }
    } else {
        // If no console specified, default is DMG.
        gameboy_type = Console::DMG;
    }

    LogLevel log_level;
    std::string log_string = Emu::GetOptionParam(tokens, "-l");
    if (!log_string.empty()) {
        if (log_string == "regular") {
            log_level = LogLevel::Regular;
        } else if (log_string == "timer") {
            log_level = LogLevel::Timer;
        } else if (log_string == "lcd") {
            log_level = LogLevel::LCD;
        } else {
            std::cerr << "Invalid log level specified: " << log_string << ". Valid levels are regular, timer, and lcd." << std::endl;
            return 1;
        }
    } else {
        // If no log level specified, then no logging by default.
        log_level = LogLevel::None;
    }

    const std::string rom_path{tokens.back()};

    std::vector<u8> rom = Emu::LoadROM(rom_path);
    if (rom.empty()) {
        // Could not open provided file, or provided file empty.
        return 1;
    }

    Emu::SDLContext sdl_context;
    if (Emu::InitSDL(sdl_context)) {
        // SDL failed to create a renderer.
        return 1;
    }

    std::ofstream log_stream = Emu::OpenLogFile(rom_path);
    if (!log_stream) {
        // Could not open log file.
        return 1;
    }

    Core::CartridgeHeader cart_header = Core::GetCartridgeHeaderInfo(gameboy_type, rom);
    Log::Logging logger(log_level, std::move(log_stream));
    Core::GameBoy gameboy_core(gameboy_type, cart_header, std::move(logger), sdl_context, std::move(rom));

    gameboy_core.EmulatorLoop();

    std::cout << "End emulation." << std::endl;
    return 0;
}
