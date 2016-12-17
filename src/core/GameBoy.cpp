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

#include "core/CartridgeHeader.h"
#include "core/GameBoy.h"

namespace Core {

GameBoy::GameBoy(const Console gb_type, const CartridgeHeader& header, Emu::SDLContext& context, std::vector<u8> rom)
        : sdl_context(context)
        , timer(Timer())
        , serial(Serial(gb_type, header.game_mode))
        , lcd(LCD())
        , joypad(Joypad())
        , mem(Memory(gb_type, header, timer, serial, lcd, joypad, std::move(rom)))
        , cpu(CPU(mem)) {

    // Link together circular dependencies after all components are constructed. For the CPU and LCD, these are
    // necessary. However, currently Timer and Serial only need a reference to Memory to request interrupts.
    lcd.LinkToGameBoy(this);
    cpu.LinkToGameBoy(this);
    timer.LinkToMemory(&mem);
    serial.LinkToMemory(&mem);
    lcd.LinkToMemory(&mem);
    joypad.LinkToMemory(&mem);
}

void GameBoy::EmulatorLoop() {
    bool quit = false, pause = false;
    while (!quit) {
        std::tie(quit, pause) = PollEvents(pause);

        if (pause) {
            SDL_Delay(40);
            continue;
        }

        // This is half the number of cycles needed for VBLANK, thus inputs are checked roughly twice a frame. Inputs
        // are still checked if the LCD is off.
        cpu.RunFor(35112);
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
                joypad.UpPressed(true);
                break;
            case SDLK_a:
                joypad.LeftPressed(true);
                break;
            case SDLK_s:
                joypad.DownPressed(true);
                break;
            case SDLK_d:
                joypad.RightPressed(true);
                break;

            case SDLK_k:
                joypad.APressed(true);
                break;
            case SDLK_j:
                joypad.BPressed(true);
                break;

            case SDLK_RETURN:
            case SDLK_i:
                joypad.StartPressed(true);
                break;
            case SDLK_u:
                joypad.SelectPressed(true);
                break;
            default:
                break;
            }
        } else if (e.type == SDL_KEYUP) {
            switch (e.key.keysym.sym) {
            case SDLK_w:
                joypad.UpPressed(false);
                break;
            case SDLK_a:
                joypad.LeftPressed(false);
                break;
            case SDLK_s:
                joypad.DownPressed(false);
                break;
            case SDLK_d:
                joypad.RightPressed(false);
                break;

            case SDLK_k:
                joypad.APressed(false);
                break;
            case SDLK_j:
                joypad.BPressed(false);
                break;

            case SDLK_RETURN:
            case SDLK_i:
                joypad.StartPressed(false);
                break;
            case SDLK_u:
                joypad.SelectPressed(false);
                break;
            default:
                break;
            }
        }
    }

    return std::make_tuple(quit, pause);
}

void GameBoy::RenderFrame(const u32* fb_ptr) {
    Emu::RenderFrame(fb_ptr, sdl_context);
}

void GameBoy::HardwareTick(unsigned int cycles) {
    for (; cycles != 0; cycles -= 4) {
        // Enable interrupts if EI was previously called.
        cpu.EnableInterruptsDelayed();

        // Update the rest of the system hardware.
        mem.UpdateOAM_DMA();
        timer.UpdateTimer();
        lcd.UpdateLCD();
        serial.UpdateSerial();
        joypad.UpdateJoypad();

        mem.IF_written_this_cycle = false;

//        timer.PrintRegisterState();
//        lcd.PrintRegisterState();
    }
}

} // End namespace Core