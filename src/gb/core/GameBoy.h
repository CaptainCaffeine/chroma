// This file is a part of Chroma.
// Copyright (C) 2016-2018 Matthew Murray
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

#include "common/CommonTypes.h"
#include "gb/core/Enums.h"

namespace Emu { class SDLContext; }

namespace Gb {

class CartridgeHeader;
class Timer;
class Serial;
class LCD;
class Joypad;
class Audio;
class Memory;
class CPU;
class Logging;

class GameBoy {
public:
    Logging& logging;

    GameBoy(const Console gb_type, const CartridgeHeader& header, Logging& logger, Emu::SDLContext& context,
            const std::string& save_file, const std::vector<u8>& rom, std::vector<u8>& save_game, bool enable_iir);
    ~GameBoy();

    void EmulatorLoop();
    void SwapBuffers(std::vector<u16>& back_buffer);
    void Screenshot() const;

    void HardwareTick(unsigned int cycles);
    void HaltedTick(unsigned int cycles);

    // Speed Switch and STOP mode functions.
    bool JoypadPress() const;
    void StopLCD();
    void SpeedSwitch();
private:
    Emu::SDLContext& sdl_context;
    std::vector<u16> front_buffer;

    const std::string save_path;

    // Game Boy hardware components.
    std::unique_ptr<Timer> timer;
    std::unique_ptr<Serial> serial;
    std::unique_ptr<LCD> lcd;
    std::unique_ptr<Joypad> joypad;
    std::unique_ptr<Audio> audio;
    std::unique_ptr<Memory> mem;
    std::unique_ptr<CPU> cpu;

    bool quit = false;
    bool pause = false;
    bool old_pause = false;

    u8 lcd_on_when_stopped = 0x00;

    void RegisterCallbacks();
};

} // End namespace Gb
