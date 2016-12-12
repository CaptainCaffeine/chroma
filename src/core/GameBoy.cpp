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
        , mem(Memory(gb_type, header, std::move(rom)))
        , timer(Timer(mem))
        , lcd(LCD(mem))
        , serial(Serial(mem))
        , cpu(CPU(mem)) {

    cpu.LinkToGameBoy(this);
}

void GameBoy::EmulatorLoop() {
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                quit = true;
            }
        }

        // This is the number of cycles needed for VBLANK. In the future, I want VBLANK to exit the RunFor function.
        cpu.RunFor(0x11250);
        Emu::RenderFrame(lcd.GetRawPointerToFramebuffer(), sdl_context);
    }

    Emu::CleanupSDL(sdl_context);
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

        mem.IF_written_this_cycle = false;

//        timer.PrintRegisterState();
//        lcd.PrintRegisterState();
    }
}

} // End namespace Core
