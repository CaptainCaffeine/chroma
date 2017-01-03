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

#pragma once

#include <memory>
#include <vector>
#include <tuple>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

namespace Log { class Logging; }
namespace Emu { struct SDLContext; }

namespace Core {

struct CartridgeHeader;
class Timer;
class Serial;
class LCD;
class Joypad;
class Memory;
class CPU;

class GameBoy {
public:
    Log::Logging& logging;

    GameBoy(const Console gb_type, const CartridgeHeader& header, Log::Logging& logger, Emu::SDLContext& context,
            std::vector<u8> rom);
    ~GameBoy();

    void EmulatorLoop();
    void SwapBuffers(std::vector<u32>& back_buffer);
    void HardwareTick(unsigned int cycles);
    void HaltedTick(unsigned int cycles);
    bool JoypadPress() const;
private:
    Emu::SDLContext& sdl_context;
    std::vector<u32> front_buffer;

    // Game Boy hardware components.
    std::unique_ptr<Timer> timer;
    std::unique_ptr<Serial> serial;
    std::unique_ptr<LCD> lcd;
    std::unique_ptr<Joypad> joypad;
    std::unique_ptr<Memory> mem;
    std::unique_ptr<CPU> cpu;

    std::tuple<bool, bool> PollEvents(bool pause);
};

} // End namespace Core
