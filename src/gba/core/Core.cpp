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

#include <chrono>

#include "gba/core/Core.h"
#include "gba/core/Enums.h"
#include "gba/memory/Memory.h"
#include "gba/cpu/Cpu.h"
#include "gba/cpu/Disassembler.h"
#include "gba/lcd/Lcd.h"
#include "gba/audio/Audio.h"
#include "gba/hardware/Timer.h"
#include "gba/hardware/Dma.h"
#include "gba/hardware/Keypad.h"
#include "gba/hardware/Serial.h"
#include "emu/SdlContext.h"
#include "common/Screenshot.h"

namespace Gba {

Core::Core(Emu::SdlContext& context, const std::vector<u32>& bios, const std::vector<u16>& rom,
           const std::string& save_path, LogLevel level)
        : mem(std::make_unique<Memory>(bios, rom, save_path, *this))
        , cpu(std::make_unique<Cpu>(*mem, *this))
        , disasm(std::make_unique<Disassembler>(level, *this))
        , lcd(std::make_unique<Lcd>(mem->PramReference(), mem->VramReference(), mem->OamReference(), *this))
        , audio(std::make_unique<Audio>(*this))
        , timers{{0, *this}, {1, *this}, {2, *this}, {3, *this}}
        , dma{{0, *this}, {1, *this}, {2, *this}, {3, *this}}
        , keypad(std::make_unique<Keypad>(*this))
        , serial(std::make_unique<Serial>(*this))
        , sdl_context(context)
        , front_buffer(Lcd::h_pixels * Lcd::v_pixels, 0x7FFF) {

    RegisterCallbacks();
}

// Needed to declare std::unique_ptr with forward-declared type in the header file.
Core::~Core() = default;

void Core::EmulatorLoop() {
    constexpr int cycles_per_frame = 279680;
    int overspent_cycles = 0;

    using namespace std::chrono;
    auto max_frame_time = 0us;
    auto avg_frame_time = 0us;
    int frame_count = 0;

    sdl_context.UnpauseAudio();

    while (!quit) {
        const auto start_time = steady_clock::now();

        sdl_context.PollEvents();

        if (pause && !frame_advance) {
            SDL_Delay(48);
            sdl_context.RenderFrame(front_buffer.data());
            continue;
        }

        frame_advance = false;

        keypad->CheckKeypadInterrupt();

        // Overspent cycles is always zero or negative.
        int target_cycles = cycles_per_frame + overspent_cycles;
        overspent_cycles = cpu->Execute(target_cycles);

        auto frame_time = duration_cast<microseconds>(steady_clock::now() - start_time);
        max_frame_time = std::max(max_frame_time, frame_time);
        avg_frame_time += frame_time;
        if (++frame_count == 60) {
            sdl_context.UpdateFrameTimes(avg_frame_time.count() / 60, max_frame_time.count());
            max_frame_time = 0us;
            avg_frame_time = 0us;
            frame_count = 0;
        }

        sdl_context.RenderFrame(front_buffer.data());
    }

    sdl_context.PauseAudio();
}

void Core::UpdateHardware(int cycles) {
    lcd_cycle_counter += cycles;
    if (lcd_cycle_counter >= next_lcd_event_cycles) {
        lcd->Update(lcd_cycle_counter);
        lcd_cycle_counter = 0;
        next_lcd_event_cycles = lcd->NextEvent();
    }

    for (int i = 0; i < 4; ++i) {
        auto& timer = timers[i];
        if (timer.TimerNotRunning()) {
            timer.InactiveTick(cycles);
        } else {
            timer_cycle_counter[i] += cycles;

            if (timer_cycle_counter[i] >= next_timer_event_cycles[i]) {
                timer.Tick(timer_cycle_counter[i]);
                timer_cycle_counter[i] = 0;
                next_timer_event_cycles[i] = timer.NextEvent();
            }
        }
    }

    audio_cycle_counter += cycles;
    if (audio_cycle_counter >= next_audio_event_cycles) {
        audio->Update(audio_cycle_counter);
        audio_cycle_counter = 0;
        next_audio_event_cycles = audio->NextEvent();
    }

    mem->DelayedSaveOp(cycles);
}

int Core::HaltCycles(int remaining_cpu_cycles) const {
    int halt_cycles = next_lcd_event_cycles - lcd_cycle_counter;

    int next_event_cycles = next_audio_event_cycles - audio_cycle_counter;
    if (next_event_cycles != 0) {
        halt_cycles = std::min(halt_cycles, next_event_cycles);
    }

    for (int i = 0; i < 4; ++i) {
        next_event_cycles = next_timer_event_cycles[i] - timer_cycle_counter[i];
        if (next_event_cycles != 0) {
            halt_cycles = std::min(halt_cycles, next_event_cycles);
        }
    }

    return std::min(halt_cycles, remaining_cpu_cycles);
}

void Core::PushBackAudio(const std::array<s16, 1600>& sample_buffer) {
    sdl_context.PushBackAudio(sample_buffer);
}

void Core::RegisterCallbacks() {
    using Emu::InputEvent;

    sdl_context.RegisterCallback(InputEvent::Quit,         [this](bool) { quit = true; });
    sdl_context.RegisterCallback(InputEvent::Pause,        [this](bool) { pause = !pause; });
    sdl_context.RegisterCallback(InputEvent::LogLevel,     [this](bool) { disasm->SwitchLogLevel(); });
    sdl_context.RegisterCallback(InputEvent::Fullscreen,   [this](bool) { sdl_context.ToggleFullscreen(); });
    sdl_context.RegisterCallback(InputEvent::Screenshot,   [this](bool) { Screenshot(); });
    sdl_context.RegisterCallback(InputEvent::LcdDebug,     [this](bool) { lcd->DumpDebugInfo(); Screenshot(); });
    sdl_context.RegisterCallback(InputEvent::HideWindow,   [this](bool) { old_pause = pause; pause = true; });
    sdl_context.RegisterCallback(InputEvent::ShowWindow,   [this](bool) { pause = old_pause; });
    sdl_context.RegisterCallback(InputEvent::FrameAdvance, [this](bool) { frame_advance = true; });

    sdl_context.RegisterCallback(InputEvent::Up,     [this](bool press) { keypad->Press(Keypad::Up, press); });
    sdl_context.RegisterCallback(InputEvent::Left,   [this](bool press) { keypad->Press(Keypad::Left, press); });
    sdl_context.RegisterCallback(InputEvent::Down,   [this](bool press) { keypad->Press(Keypad::Down, press); });
    sdl_context.RegisterCallback(InputEvent::Right,  [this](bool press) { keypad->Press(Keypad::Right, press); });
    sdl_context.RegisterCallback(InputEvent::A,      [this](bool press) { keypad->Press(Keypad::A, press); });
    sdl_context.RegisterCallback(InputEvent::B,      [this](bool press) { keypad->Press(Keypad::B, press); });
    sdl_context.RegisterCallback(InputEvent::L,      [this](bool press) { keypad->Press(Keypad::L, press); });
    sdl_context.RegisterCallback(InputEvent::R,      [this](bool press) { keypad->Press(Keypad::R, press); });
    sdl_context.RegisterCallback(InputEvent::Start,  [this](bool press) { keypad->Press(Keypad::Start, press); });
    sdl_context.RegisterCallback(InputEvent::Select, [this](bool press) { keypad->Press(Keypad::Select, press); });
}

void Core::Screenshot() const {
    Common::WritePPMFile(Common::BGR5ToRGB8(front_buffer), "screenshot.ppm", 240, 160);
}

} // End namespace Gba
