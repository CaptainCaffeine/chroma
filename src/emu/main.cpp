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
#include "gb/core/Enums.h"
#include "gb/core/GameBoy.h"
#include "gb/logging/Logging.h"
#include "gb/memory/CartridgeHeader.h"
#include "gba/core/Core.h"
#include "gba/memory/Memory.h"
#include "emu/ParseOptions.h"
#include "emu/SDLContext.h"

int main(int argc, char** argv) {
    std::vector<std::string> tokens = Emu::GetTokens(argv, argv + argc);

    if (tokens.size() == 1 || Emu::ContainsOption(tokens, "-h")) {
        Emu::DisplayHelp();
        return 1;
    }

    Gb::Console gameboy_type;
    Gb::LogLevel log_level;
    unsigned int pixel_scale;
    bool enable_iir;
    bool fullscreen;
    bool multicart;
    try {
        gameboy_type = Emu::GetGameBoyType(tokens);
        log_level = Emu::GetLogLevel(tokens);
        pixel_scale = Emu::GetPixelScale(tokens);
        enable_iir = Emu::GetFilterEnable(tokens);
        fullscreen = Emu::ContainsOption(tokens, "-f");
        multicart = Emu::ContainsOption(tokens, "--multicart");
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << "\n\n";
        Emu::DisplayHelp();
        return 1;
    }

    try {
        const std::string rom_path{tokens.back()};

        if (Emu::CheckRomFile(rom_path) == Gb::Console::AGB) {
            const std::vector<u32> bios{Emu::LoadGbaBios()};
            const std::vector<u16> rom{Emu::LoadRom<u16>(rom_path)};
            Gba::Memory::CheckHeader(rom);

            Emu::SDLContext sdl_context{240, 160, pixel_scale, fullscreen};
            Gba::Core gba_core{sdl_context, bios, rom};

            gba_core.EmulatorLoop();
        } else {
            const std::vector<u8> rom{Emu::LoadRom<u8>(rom_path)};
            const Gb::CartridgeHeader cart_header{gameboy_type, rom, multicart};

            const std::string save_path{Emu::SaveGamePath(rom_path)};
            std::vector<u8> save_game{Emu::LoadSaveGame(cart_header, save_path)};

            Gb::Logging logger{log_level};
            Emu::SDLContext sdl_context{160, 144, pixel_scale, fullscreen};
            Gb::GameBoy gameboy_core{gameboy_type, cart_header, logger, sdl_context, save_path, rom, save_game,
                                     enable_iir};

            gameboy_core.EmulatorLoop();
        }
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
