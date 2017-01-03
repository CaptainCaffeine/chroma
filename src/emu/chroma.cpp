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
#include <stdexcept>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"
#include "common/logging/Logging.h"
#include "core/CartridgeHeader.h"
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
    LogLevel log_level;
    try {
        gameboy_type = Emu::GetGameBoyType(tokens);
        log_level = Emu::GetLogLevel(tokens);
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << "\n\n";
        Emu::DisplayHelp();
        return 1;
    }

    const std::string rom_path{tokens.back()};

    std::vector<u8> rom;
    try {
        rom = Emu::LoadROM(rom_path);
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    Emu::SDLContext sdl_context;
    try {
        Emu::InitSDL(sdl_context);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    std::ofstream log_stream;
    // Leave log_stream unopened if logging disabled.
    if (log_level != LogLevel::None) {
        try {
            log_stream = Emu::OpenLogFile(rom_path);
        } catch (const std::invalid_argument& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }
    }

    Core::CartridgeHeader cart_header = Core::GetCartridgeHeaderInfo(gameboy_type, rom);
    Log::Logging logger(log_level, std::move(log_stream));
    Core::GameBoy gameboy_core(gameboy_type, cart_header, logger, sdl_context, std::move(rom));

    try {
        gameboy_core.EmulatorLoop();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
    }

    std::cout << "End emulation." << std::endl;
    return 0;
}
