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

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"
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
    std::string mode_string = Emu::GetOptionParam(tokens, "-m");
    if (!mode_string.empty()) {
        if (mode_string == "dmg") {
            gameboy_type = Console::DMG;
        } else if (mode_string == "cgb") {
            gameboy_type = Console::CGB;
        } else {
            std::cerr << "Invalid mode specified: " << mode_string << ". Valid modes are dmg and cgb." << std::endl;
            return 1;
        }
    } else {
        // If no console specified, default is DMG.
        gameboy_type = Console::DMG;
    }

    std::vector<u8> rom = Emu::LoadROM(tokens.back());
    if (rom.empty()) {
        // Could not open provided file, or provided file empty.
        return 1;
    } else if (rom.size() < 0x8000) {
        std::cerr << "Rom size of " << rom.size() << " bytes is too small to be a Game Boy game." << std::endl;
        return 1;
    }

    Emu::SDLContext sdl_context;
    if (Emu::InitSDL(sdl_context)) {
        // SDL failed to create a renderer.
        return 1;
    }

    Core::CartridgeHeader cart_header = Core::GetCartridgeHeaderInfo(gameboy_type, rom);
    Core::GameBoy gameboy_core(gameboy_type, cart_header, sdl_context, std::move(rom));

    gameboy_core.EmulatorLoop();

    std::cout << "End emulation." << std::endl;
    return 0;
}
