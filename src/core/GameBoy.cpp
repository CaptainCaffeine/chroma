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
#include "common/Util.h"
#include "core/CartridgeHeader.h"
#include "core/GameBoy.h"
#include "core/memory/Memory.h"
#include "core/Timer.h"
#include "core/Serial.h"
#include "core/lcd/LCD.h"
#include "core/Joypad.h"
#include "core/cpu/CPU.h"
#include "emu/SDLContext.h"

namespace Core {

GameBoy::GameBoy(const Console gb_type, const CartridgeHeader& header, Log::Logging& logger, Emu::SDLContext& context,
                 const std::string& save_file, const std::vector<u8>& rom, std::vector<u8>& save_game)
        : logging(logger)
        , sdl_context(context)
        , front_buffer(160*144)
        , save_path(save_file)
        , timer(new Timer())
        , serial(new Serial())
        , lcd(new LCD())
        , joypad(new Joypad())
        , mem(new Memory(gb_type, header, *timer, *serial, *lcd, *joypad, rom, save_game))
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

GameBoy::~GameBoy() {
    mem->SaveExternalRAM(save_path);
}

void GameBoy::EmulatorLoop() {
    int overspent_cycles = 0;
    bool quit = false, pause = false;
    while (!quit) {
        std::tie(quit, pause) = PollEvents(pause);

        if (pause) {
            SDL_Delay(40);
            continue;
        }

        // 70224 is the number of cycles between VBLANKs, when the LCD is on.
        // Overspent cycles is always zero or negative.
        overspent_cycles = cpu->RunFor((70224 << mem->double_speed) + overspent_cycles);

        sdl_context.RenderFrame(front_buffer.data());
    }
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
                if (e.key.repeat == 0) {
                    pause = !pause;
                }
                break;
            case SDLK_b:
                if (e.key.repeat == 0) {
                    logging.SwitchLogLevel();
                }
                break;
            case SDLK_v:
                if (e.key.repeat == 0) {
                    sdl_context.ToggleFullscreen();
                }
                break;
            case SDLK_t:
                if (e.key.repeat == 0) {
                    Screenshot();
                }
                break;
            case SDLK_y:
                if (e.key.repeat == 0) {
                    lcd->DumpEverything();
                }
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
        lcd->UpdateLCD();
        serial->UpdateSerial();
        joypad->UpdateJoypad();

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
        lcd->UpdateLCD();
        serial->UpdateSerial();
        joypad->UpdateJoypad();
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

} // End namespace Core
