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

#include "gba/core/Core.h"
#include "gba/core/Enums.h"
#include "gba/memory/Memory.h"
#include "gba/cpu/Cpu.h"
#include "gba/lcd/Lcd.h"
#include "gba/hardware/Timer.h"
#include "gba/hardware/Dma.h"
#include "gba/hardware/Keypad.h"
#include "emu/SDLContext.h"

namespace Gba {

Core::Core(Emu::SDLContext& context, const std::vector<u32>& bios, const std::vector<u16>& rom, LogLevel level)
        : mem(std::make_unique<Memory>(bios, rom, *this))
        , cpu(std::make_unique<Cpu>(*mem, *this, level))
        , lcd(std::make_unique<Lcd>(*this))
        , timers{{0, *this}, {1, *this}, {2, *this}, {3, *this}}
        , dma{{0, *this}, {1, *this}, {2, *this}, {3, *this}}
        , keypad(std::make_unique<Keypad>(*this))
        , sdl_context(context) {

    RegisterCallbacks();
}

// Needed to declare std::unique_ptr with forward-declared type in the header file.
Core::~Core() = default;

void Core::EmulatorLoop() {
    cpu->Execute(0x2000000);

    //while (!quit) {
    //    sdl_context.PollEvents();

    //    if (pause) {
    //        SDL_Delay(40);
    //        continue;
    //    }

    //    keypad->CheckKeypadInterrupt();
    //    cpu->Execute(0x1000000);
    //}
}

void Core::UpdateHardware(int cycles) {
    if (cycles == 0) {
        return;
    }

    lcd->Update(cycles);

    for (auto& timer : timers) {
        timer.Tick(cycles);
    }
}

void Core::RegisterCallbacks() {
    using Emu::InputEvent;

    sdl_context.RegisterCallback(InputEvent::Quit,       [this](bool) { quit = true; });
    sdl_context.RegisterCallback(InputEvent::Pause,      [this](bool) { pause = !pause; });
    sdl_context.RegisterCallback(InputEvent::LogLevel,   [](bool) { });
    sdl_context.RegisterCallback(InputEvent::Fullscreen, [this](bool) { sdl_context.ToggleFullscreen(); });
    sdl_context.RegisterCallback(InputEvent::Screenshot, [](bool) { });
    sdl_context.RegisterCallback(InputEvent::LcdDebug,   [](bool) { });
    sdl_context.RegisterCallback(InputEvent::Up,         [this](bool press) { keypad->Press(Keypad::Up, press); });
    sdl_context.RegisterCallback(InputEvent::Left,       [this](bool press) { keypad->Press(Keypad::Left, press); });
    sdl_context.RegisterCallback(InputEvent::Down,       [this](bool press) { keypad->Press(Keypad::Down, press); });
    sdl_context.RegisterCallback(InputEvent::Right,      [this](bool press) { keypad->Press(Keypad::Right, press); });
    sdl_context.RegisterCallback(InputEvent::A,          [this](bool press) { keypad->Press(Keypad::A, press); });
    sdl_context.RegisterCallback(InputEvent::B,          [this](bool press) { keypad->Press(Keypad::B, press); });
    sdl_context.RegisterCallback(InputEvent::L,          [this](bool press) { keypad->Press(Keypad::L, press); });
    sdl_context.RegisterCallback(InputEvent::R,          [this](bool press) { keypad->Press(Keypad::R, press); });
    sdl_context.RegisterCallback(InputEvent::Start,      [this](bool press) { keypad->Press(Keypad::Start, press); });
    sdl_context.RegisterCallback(InputEvent::Select,     [this](bool press) { keypad->Press(Keypad::Select, press); });
}

} // End namespace Gba
