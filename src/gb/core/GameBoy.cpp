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

#include "common/Screenshot.h"
#include "gb/core/GameBoy.h"
#include "gb/cpu/CPU.h"
#include "gb/memory/Memory.h"
#include "gb/memory/CartridgeHeader.h"
#include "gb/lcd/LCD.h"
#include "gb/audio/Audio.h"
#include "gb/hardware/Timer.h"
#include "gb/hardware/Serial.h"
#include "gb/hardware/Joypad.h"
#include "gb/logging/Logging.h"
#include "emu/SDLContext.h"

namespace Gb {

GameBoy::GameBoy(const Console gb_type, const CartridgeHeader& header, Logging& logger, Emu::SDLContext& context,
                 const std::string& save_file, const std::vector<u8>& rom, std::vector<u8>& save_game, bool enable_iir)
        : logging(logger)
        , sdl_context(context)
        , front_buffer(160*144)
        , save_path(save_file)
        , timer(std::make_unique<Timer>())
        , serial(std::make_unique<Serial>())
        , lcd(std::make_unique<LCD>())
        , joypad(std::make_unique<Joypad>())
        , audio(std::make_unique<Audio>(enable_iir))
        , mem(std::make_unique<Memory>(gb_type, header, *timer, *serial, *lcd, *joypad, *audio, rom, save_game))
        , cpu(std::make_unique<CPU>(*mem)) {

    // Link together circular dependencies after all components are constructed.
    lcd->LinkToGameBoy(this);
    cpu->LinkToGameBoy(this);
    timer->LinkToMemory(mem.get());
    serial->LinkToMemory(mem.get());
    lcd->LinkToMemory(mem.get());
    joypad->LinkToMemory(mem.get());
    audio->LinkToMemory(mem.get());

    RegisterCallbacks();
}

GameBoy::~GameBoy() {
    mem->SaveExternalRAM(save_path);
}

void GameBoy::EmulatorLoop() {
    constexpr int cycles_per_frame = 70224;
    int overspent_cycles = 0;

    sdl_context.UnpauseAudio();

    while (!quit) {
        sdl_context.PollEvents();

        if (pause) {
            SDL_Delay(48);
            continue;
        }

        // Overspent cycles is always zero or negative.
        int target_cycles = (cycles_per_frame << mem->double_speed) + overspent_cycles;
        overspent_cycles = cpu->RunFor(target_cycles);

        sdl_context.PushBackAudio(audio->output_buffer);
        sdl_context.RenderFrame(front_buffer.data());
    }

    sdl_context.PauseAudio();
}

void GameBoy::RegisterCallbacks() {
    using Emu::InputEvent;

    sdl_context.RegisterCallback(InputEvent::Quit,       [this](bool) { quit = true; });
    sdl_context.RegisterCallback(InputEvent::Pause,      [this](bool) { pause = !pause; });
    sdl_context.RegisterCallback(InputEvent::LogLevel,   [this](bool) { logging.SwitchLogLevel(); });
    sdl_context.RegisterCallback(InputEvent::Fullscreen, [this](bool) { sdl_context.ToggleFullscreen(); });
    sdl_context.RegisterCallback(InputEvent::Screenshot, [this](bool) { Screenshot(); });
    sdl_context.RegisterCallback(InputEvent::LcdDebug,   [this](bool) { lcd->DumpEverything(); });
    sdl_context.RegisterCallback(InputEvent::Up,         [this](bool press) { joypad->UpPressed(press); });
    sdl_context.RegisterCallback(InputEvent::Left,       [this](bool press) { joypad->LeftPressed(press); });
    sdl_context.RegisterCallback(InputEvent::Down,       [this](bool press) { joypad->DownPressed(press); });
    sdl_context.RegisterCallback(InputEvent::Right,      [this](bool press) { joypad->RightPressed(press); });
    sdl_context.RegisterCallback(InputEvent::A,          [this](bool press) { joypad->APressed(press); });
    sdl_context.RegisterCallback(InputEvent::B,          [this](bool press) { joypad->BPressed(press); });
    sdl_context.RegisterCallback(InputEvent::L,          [](bool) { });
    sdl_context.RegisterCallback(InputEvent::R,          [](bool) { });
    sdl_context.RegisterCallback(InputEvent::Start,      [this](bool press) { joypad->StartPressed(press); });
    sdl_context.RegisterCallback(InputEvent::Select,     [this](bool press) { joypad->SelectPressed(press); });
}

void GameBoy::SwapBuffers(std::vector<u16>& back_buffer) {
    front_buffer.swap(back_buffer);
}

void GameBoy::Screenshot() const {
    Common::WritePPMFile(Common::BGR5ToRGB8(front_buffer), "screenshot.ppm", 160, 144);
}

void GameBoy::HardwareTick(unsigned int cycles) {
    for (; cycles != 0; cycles -= 4) {
        // Log I/O registers if logging enabled.
        if (logging.log_level == LogLevel::Timer) {
            logging.LogTimerRegisterState(*timer);
        } else if (logging.log_level == LogLevel::LCD) {
            logging.LogLCDRegisterState(*lcd);
        }

        // Enable interrupts if EI was previously called.
        cpu->EnableInterruptsDelayed();

        // Update the rest of the system hardware.
        mem->UpdateOAM_DMA();
        mem->UpdateHDMA();
        timer->UpdateTimer();
        serial->UpdateSerial();
        lcd->UpdateLCD();
        joypad->UpdateJoypad();

        // The APU always updates at 2MHz, regardless of double speed mode. So we need to update it twice an M-cycle
        // in single-speed mode.
        for (int i = 0; i < (2 >> mem->double_speed); ++i) {
            audio->UpdateAudio();
        }

        mem->IF_written_this_cycle = false;
    }
}

void GameBoy::HaltedTick(unsigned int cycles) {
    for (; cycles != 0; cycles -= 4) {
        // Log I/O registers if logging enabled.
        if (logging.log_level == LogLevel::Timer) {
            logging.LogTimerRegisterState(*timer);
        } else if (logging.log_level == LogLevel::LCD) {
            logging.LogLCDRegisterState(*lcd);
        }

        // Update the rest of the system hardware.
        timer->UpdateTimer();
        serial->UpdateSerial();
        lcd->UpdateLCD();
        joypad->UpdateJoypad();

        // The APU always updates at 2MHz, regardless of double speed mode. So we need to update it twice an M-cycle
        // in single-speed mode.
        for (int i = 0; i < (2 >> mem->double_speed); ++i) {
            audio->UpdateAudio();
        }
    }
}

bool GameBoy::JoypadPress() const {
    return joypad->JoypadPress();
}

void GameBoy::StopLCD() {
    // Record the current LCD power state for when we exit stop mode.
    lcd_on_when_stopped = lcd->lcdc & 0x80;

    // Turn off the LCD.
    lcd->lcdc &= 0x7F;
}

void GameBoy::SpeedSwitch() {
    mem->ToggleCPUSpeed();

    // If the LCD was on when we entered STOP mode, turn it back on.
    lcd->lcdc |= lcd_on_when_stopped;
}

} // End namespace Gb
