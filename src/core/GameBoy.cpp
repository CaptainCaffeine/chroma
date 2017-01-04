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

#include "common/logging/Logging.h"
#include "core/CartridgeHeader.h"
#include "core/GameBoy.h"
#include "core/memory/Memory.h"
#include "core/Timer.h"
#include "core/Serial.h"
#include "core/LCD.h"
#include "core/Joypad.h"
#include "core/cpu/CPU.h"
#include "emu/SDL_Utils.h"

namespace Core {

GameBoy::GameBoy(const Console gb_type, const CartridgeHeader& header, Log::Logging& logger, Emu::SDLContext& context,
                 std::vector<u8> rom)
        : logging(logger)
        , sdl_context(context)
        , front_buffer(160*144)
        , timer(new Timer())
        , serial(new Serial(gb_type, header.game_mode))
        , lcd(new LCD())
        , joypad(new Joypad())
        , mem(new Memory(gb_type, header, *timer, *serial, *lcd, *joypad, std::move(rom)))
        , cpu(new CPU(*mem)) {

    // Link together circular dependencies after all components are constructed. For the CPU and LCD, these are
    // necessary. However, currently Timer and Serial only need a reference to Memory to request interrupts.
    lcd->LinkToGameBoy(this);
    cpu->LinkToGameBoy(this);
    timer->LinkToMemory(mem.get());
    serial->LinkToMemory(mem.get());
    lcd->LinkToMemory(mem.get());
    joypad->LinkToMemory(mem.get());
}

// Needed to use forward declarations as template parameters in GameBoy.h.
GameBoy::~GameBoy() = default;

void GameBoy::EmulatorLoop() {
    bool quit = false, pause = false;
    while (!quit) {
        std::tie(quit, pause) = PollEvents(pause);

        if (pause) {
            SDL_Delay(40);
            continue;
        }

        // This is the number of cycles between VBLANKs, when the LCD is on.
        cpu->RunFor(70224);

        Emu::RenderFrame(front_buffer.data(), sdl_context);
    }

    Emu::CleanupSDL(sdl_context);
}

std::tuple<bool, bool> GameBoy::PollEvents(bool pause) {
    SDL_Event e;
    bool quit = false;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quit = true;
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
            case SDLK_q:
            case SDLK_ESCAPE:
                quit = true;
                break;
            case SDLK_p:
                pause = !pause;
                break;

            case SDLK_w:
                joypad->UpPressed(true);
                break;
            case SDLK_a:
                joypad->LeftPressed(true);
                break;
            case SDLK_s:
                joypad->DownPressed(true);
                break;
            case SDLK_d:
                joypad->RightPressed(true);
                break;

            case SDLK_k:
                joypad->APressed(true);
                break;
            case SDLK_j:
                joypad->BPressed(true);
                break;

            case SDLK_RETURN:
            case SDLK_i:
                joypad->StartPressed(true);
                break;
            case SDLK_u:
                joypad->SelectPressed(true);
                break;
            default:
                break;
            }
        } else if (e.type == SDL_KEYUP) {
            switch (e.key.keysym.sym) {
            case SDLK_w:
                joypad->UpPressed(false);
                break;
            case SDLK_a:
                joypad->LeftPressed(false);
                break;
            case SDLK_s:
                joypad->DownPressed(false);
                break;
            case SDLK_d:
                joypad->RightPressed(false);
                break;

            case SDLK_k:
                joypad->APressed(false);
                break;
            case SDLK_j:
                joypad->BPressed(false);
                break;

            case SDLK_RETURN:
            case SDLK_i:
                joypad->StartPressed(false);
                break;
            case SDLK_u:
                joypad->SelectPressed(false);
                break;
            default:
                break;
            }
        }
    }

    return std::make_tuple(quit, pause);
}

void GameBoy::SwapBuffers(std::vector<u32>& back_buffer) {
    front_buffer.swap(back_buffer);
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
        timer->UpdateTimer();
        lcd->UpdateLCD();
        serial->UpdateSerial();
        joypad->UpdateJoypad();

        mem->IF_written_this_cycle = false;
    }
}

void GameBoy::HaltedTick(unsigned int cycles) {
    for (; cycles != 0; cycles -= 4) {
        // Update the rest of the system hardware.
        timer->UpdateTimer();
        lcd->UpdateLCD();
        serial->UpdateSerial();
        joypad->UpdateJoypad();

        // Log I/O registers if logging enabled.
        if (logging.log_level == LogLevel::Timer) {
            logging.LogTimerRegisterState(*timer);
        } else if (logging.log_level == LogLevel::LCD) {
            logging.LogLCDRegisterState(*lcd);
        }
    }
}

bool GameBoy::JoypadPress() const {
    return joypad->signal_went_low;
}

} // End namespace Core
