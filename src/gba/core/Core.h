// This file is a part of Chroma.
// Copyright (C) 2017-2018 Matthew Murray
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
#include <string>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

namespace Emu { class SDLContext; }

namespace Gba {

class Memory;
class Cpu;
class Lcd;
class Timer;
class Dma;
class Keypad;
class Serial;

class Core {
public:
    Core(Emu::SDLContext& context, const std::vector<u32>& bios, const std::vector<u16>& rom,
         const std::string& save_path, LogLevel level);
    ~Core();

    std::unique_ptr<Memory> mem;
    std::unique_ptr<Cpu> cpu;
    std::unique_ptr<Lcd> lcd;
    std::vector<Timer> timers;
    std::vector<Dma> dma;
    std::unique_ptr<Keypad> keypad;
    std::unique_ptr<Serial> serial;

    void EmulatorLoop();
    void UpdateHardware(int cycles);
    void SwapBuffers(std::vector<u16>& back_buffer) { front_buffer.swap(back_buffer); }
private:
    Emu::SDLContext& sdl_context;
    std::vector<u16> front_buffer;

    bool quit = false;
    bool pause = false;
    bool old_pause = false;

    void RegisterCallbacks();
};

} // End namespace Gba
