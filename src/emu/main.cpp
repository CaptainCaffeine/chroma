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

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"
#include "common/logging/Logging.h"
#include "core/CartridgeHeader.h"
#include "core/GameBoy.h"
#include "emu/ParseOptions.h"
#include "emu/SDLContext.h"

int main(int argc, char** argv) {
    std::vector<std::string> tokens = Emu::GetTokens(argv, argv+argc);

    if (tokens.size() == 1 || Emu::ContainsOption(tokens, "-h")) {
        Emu::DisplayHelp();
        return 1;
    }

    Console gameboy_type;
    LogLevel log_level;
    unsigned int pixel_scale;
    bool fullscreen;
    bool multicart;
    try {
        gameboy_type = Emu::GetGameBoyType(tokens);
        log_level = Emu::GetLogLevel(tokens);
        pixel_scale = Emu::GetPixelScale(tokens);
        fullscreen = Emu::ContainsOption(tokens, "-f");
        multicart = Emu::ContainsOption(tokens, "--multicart");
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << "\n\n";
        Emu::DisplayHelp();
        return 1;
    }

    try {
        const std::string rom_path{tokens.back()};
        const std::vector<u8> rom{Emu::LoadROM(rom_path)};

        const Core::CartridgeHeader cart_header{gameboy_type, rom, multicart};

        std::string save_path{Emu::SaveGamePath(rom_path)};
        std::vector<u8> save_game{Emu::LoadSaveGame(cart_header, save_path)};

        Log::Logging logger{log_level};
        Emu::SDLContext sdl_context{pixel_scale, fullscreen};
        Core::GameBoy gameboy_core{gameboy_type, cart_header, logger, sdl_context, save_path, rom, save_game};

        gameboy_core.EmulatorLoop();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
