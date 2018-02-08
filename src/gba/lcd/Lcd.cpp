// This file is a part of Chroma.
// Copyright (C) 2018 Matthew Murray
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

#include "gba/lcd/Lcd.h"
#include "gba/core/Core.h"
#include "gba/core/Enums.h"
#include "gba/memory/Memory.h"

namespace Gba {

Lcd::Lcd(Core& _core)
        : core(_core) {}

void Lcd::Update(int cycles) {
    int updated_cycles = scanline_cycles + cycles;

    if (scanline_cycles < 960 && updated_cycles >= 960) {
        // Begin hblank.
        if (dispstat & hblank_irq) {
            core.mem->RequestInterrupt(Interrupt::HBlank);
        }
    } else if (scanline_cycles < 1006 && updated_cycles >= 1006) {
        // The hblank flag isn't set until 46 cycles into the hblank period.
        dispstat |= hblank_flag;
    } else if (updated_cycles >= 1232) {
        updated_cycles -= 1232;

        dispstat &= ~hblank_flag;

        if (++vcount == 160) {
            // Begin vblank.
            dispstat |= vblank_flag;

            if (dispstat & vblank_irq) {
                core.mem->RequestInterrupt(Interrupt::VBlank);
            }
        } else if (vcount == 227) {
            // Vblank flag is unset one scanline before vblank ends.
            dispstat &= ~vblank_flag;
        } else if (vcount == 228) {
            // Start new frame.
            vcount = 0;
        }

        if (vcount == VTrigger()) {
            dispstat |= vcount_flag;

            if (dispstat & vcount_irq) {
                core.mem->RequestInterrupt(Interrupt::VCount);
            }
        } else {
            dispstat &= ~vcount_flag;
        }
    }

    scanline_cycles = updated_cycles;
}

} // End namespace Gba
