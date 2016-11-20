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
#include "core/Memory.h"
#include "core/Timer.h"
#include "core/CPU.h"
#include "emu/ParseOptions.h"

int main(int argc, char** argv) {
    std::vector<std::string> tokens = Emu::GetTokens(argv, argv+argc);

    if (tokens.size() == 1 || Emu::ContainsOption(tokens, "-h")) {
        Emu::DisplayHelp();
        return 1;
    }

    Console game_boy;
    std::string mode_string = Emu::GetOptionParam(tokens, "-m");
    if (!mode_string.empty()) {
        if (mode_string == "dmg") {
            game_boy = Console::DMG;
        } else if (mode_string == "cgb") {
            game_boy = Console::CGB;
        } else {
            std::cerr << "Invalid mode specified: " << mode_string << ". Valid modes are dmg and cgb." << std::endl;
            return 1;
        }
    } else {
        // If no console specified, default is DMG.
        game_boy = Console::DMG;
    }

    std::vector<u8> rom = Emu::LoadROM(tokens.back());
    if (rom.empty()) {
        // Could not open provided file, or provided file empty.
        return 1;
    } else if (rom.size() < 0x8000) {
        std::cerr << "Rom size of " << rom.size() << " bytes is too small to be a Game Boy game." << std::endl;
        return 1;
    }

    // Init
    Core::CartridgeHeader cart_header = Core::GetCartridgeHeaderInfo(game_boy, rom);
    Core::Memory memory(game_boy, cart_header, std::move(rom));
    Core::Timer timer(memory);
    Core::CPU cpu(memory, timer);

    while (true) {
        // Eventually the cycle number will be synced with lcd or sound timings.
        cpu.RunFor(0x80000);
    }

    std::cout << "End emulation." << std::endl;
    return 0;
}
