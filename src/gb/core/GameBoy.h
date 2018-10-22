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
#include <string>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"
#include "gb/core/Enums.h"

namespace Emu { class SdlContext; }

namespace Gb {

class CartridgeHeader;
class Timer;
class Serial;
class Lcd;
class Joypad;
class Audio;
class Memory;
class Cpu;
class Logging;

class GameBoy {
public:
    GameBoy(const Console _console, const CartridgeHeader& header, Emu::SdlContext& context,
            const std::string& save_path, const std::vector<u8>& rom, bool enable_iir, LogLevel log_level);
    ~GameBoy();

    const Console console;
    const GameMode game_mode;

    std::unique_ptr<Timer> timer;
    std::unique_ptr<Serial> serial;
    std::unique_ptr<Lcd> lcd;
    std::unique_ptr<Joypad> joypad;
    std::unique_ptr<Audio> audio;
    std::unique_ptr<Memory> mem;
    std::unique_ptr<Cpu> cpu;
    std::unique_ptr<Logging> logging;

    void EmulatorLoop();
    void SwapBuffers(std::vector<u16>& back_buffer);
    void Screenshot() const;

    void HardwareTick(unsigned int cycles);
    void HaltedTick(unsigned int cycles);

    bool ConsoleDmg() const { return console == Console::DMG; }
    bool ConsoleCgb() const { return console == Console::CGB || console == Console::AGB; }
    bool GameModeDmg() const { return game_mode == GameMode::DMG; }
    bool GameModeCgb() const { return game_mode == GameMode::CGB; }

    // Speed Switch and STOP mode functions.
    bool JoypadPress() const;
    void StopLcd();
    void SpeedSwitch();

private:
    Emu::SdlContext& sdl_context;
    std::vector<u16> front_buffer;

    bool quit = false;
    bool pause = false;
    bool old_pause = false;
    bool frame_advance = false;

    u8 lcd_on_when_stopped = 0x00;

    void RegisterCallbacks();
};

} // End namespace Gb
