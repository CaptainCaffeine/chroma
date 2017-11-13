// This file is a part of Chroma.
// Copyright (C) 2017 Matthew Murray
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
#include "gba/memory/Memory.h"
#include "gba/cpu/Cpu.h"
#include "emu/SDLContext.h"

namespace Gba {

Core::Core(Emu::SDLContext& context, const std::vector<u16>& rom)
        : sdl_context(context)
        , mem(std::make_unique<Memory>(rom))
        , cpu(std::make_unique<Cpu>(*mem)) {

    RegisterCallbacks();
}

// Needed to declare unique_ptrs with forward declarations in the header file.
Core::~Core() = default;

void Core::EmulatorLoop() {
    cpu->Execute(20);
    //while (!quit) {
    //    sdl_context.PollEvents();

    //    SDL_Delay(40);
    //}
}

void Core::RegisterCallbacks() {
    using Emu::InputEvent;

    sdl_context.RegisterCallback(InputEvent::Quit,       [this](bool) { quit = true; });
    sdl_context.RegisterCallback(InputEvent::Pause,      [this](bool) { pause = !pause; });
    sdl_context.RegisterCallback(InputEvent::LogLevel,   [](bool) { });
    sdl_context.RegisterCallback(InputEvent::Fullscreen, [this](bool) { sdl_context.ToggleFullscreen(); });
    sdl_context.RegisterCallback(InputEvent::Screenshot, [](bool) { });
    sdl_context.RegisterCallback(InputEvent::LcdDebug,   [](bool) { });
    sdl_context.RegisterCallback(InputEvent::Up,         [](bool) { });
    sdl_context.RegisterCallback(InputEvent::Left,       [](bool) { });
    sdl_context.RegisterCallback(InputEvent::Down,       [](bool) { });
    sdl_context.RegisterCallback(InputEvent::Right,      [](bool) { });
    sdl_context.RegisterCallback(InputEvent::A,          [](bool) { });
    sdl_context.RegisterCallback(InputEvent::B,          [](bool) { });
    sdl_context.RegisterCallback(InputEvent::L,          [](bool) { });
    sdl_context.RegisterCallback(InputEvent::R,          [](bool) { });
    sdl_context.RegisterCallback(InputEvent::Start,      [](bool) { });
    sdl_context.RegisterCallback(InputEvent::Select,     [](bool) { });
}

} // End namespace Gba
