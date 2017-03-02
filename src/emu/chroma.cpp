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

    const std::string rom_path{tokens.back()};
    std::vector<u8> rom;
    try {
        rom = Emu::LoadROM(rom_path);
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    std::ofstream log_stream;
    // Leave log_stream unopened if logging disabled.
    if (log_level != LogLevel::None) {
        log_stream = std::ofstream("log.txt");
        if (!log_stream) {
            std::cerr << "Error when attempting to open ./log.txt for writing.\n";
            return 1;
        }
    }

    Core::CartridgeHeader cart_header = Core::GetCartridgeHeaderInfo(gameboy_type, rom, multicart);

    std::string save_path;
    std::vector<u8> save_game;
    if (cart_header.ext_ram_present) {
        try {
            save_path = Emu::SaveGamePath(rom_path);
            save_game = Emu::LoadSaveGame(save_path);
        } catch (const std::invalid_argument& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }

        if (save_game.size() != 0) {
            unsigned int cart_ram_size = cart_header.ram_size;
            if (cart_header.rtc_present) {
                // Account for RTC save data at end of save file.
                cart_ram_size += 48;
            }

            if (cart_ram_size != save_game.size()) {
                std::cerr << "Save game size does not match external RAM size given in cartridge header.\n";
                return 1;
            }
        }
    }

    Emu::SDLContext sdl_context;
    try {
        Emu::InitSDL(sdl_context, pixel_scale, fullscreen);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    Log::Logging logger(log_level, std::move(log_stream));
    Core::GameBoy gameboy_core(gameboy_type, cart_header, logger, sdl_context, std::move(rom), std::move(save_game));

    try {
        gameboy_core.EmulatorLoop();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
    }

    if (cart_header.ext_ram_present) {
        std::ofstream save_ostream(save_path);
        if (!save_ostream) {
            std::cerr << "Error: could not open " << save_path << " to write save file to disk.\n";
        } else {
            gameboy_core.WriteSaveFile(save_ostream);
            save_ostream.flush();
        }
    }

    Emu::CleanupSDL(sdl_context);

    std::cout << "End emulation." << std::endl;
    return 0;
}
