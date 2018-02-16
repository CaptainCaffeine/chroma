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

#include <array>
#include <algorithm>

#include "gba/lcd/Lcd.h"
#include "gba/lcd/Bg.h"
#include "gba/core/Core.h"
#include "gba/core/Enums.h"
#include "gba/memory/Memory.h"
#include "gba/hardware/Dma.h"

namespace Gba {

Lcd::Lcd(const std::vector<u16>& _pram, const std::vector<u16>& _vram, const std::vector<u32>& _oam, Core& _core)
        : bgs{*this, *this, *this, *this}
        , pram(_pram)
        , vram(_vram)
        , oam(_oam)
        , core(_core)
        , back_buffer(h_pixels * v_pixels, 0x7FFF) {}

// Needed to declare std::vector with forward-declared type in the header file.
Lcd::~Lcd() = default;

void Lcd::Update(int cycles) {
    int updated_cycles = scanline_cycles + cycles;

    if (scanline_cycles < 960 && updated_cycles >= 960) {
        // Begin hblank.
        if (status & hblank_irq) {
            core.mem->RequestInterrupt(Interrupt::HBlank);
        }

        // Trigger the HBlank and Video Capture DMAs, if any are pending.
        if (vcount < 160) {
            DrawScanline();

            for (auto& dma : core.dma) {
                dma.Trigger(Dma::HBlank);
            }
        }

        if (vcount > 1 && vcount < 162) {
            core.dma[3].Trigger(Dma::Special);
        }
    } else if (scanline_cycles < 1006 && updated_cycles >= 1006) {
        // The hblank flag isn't set until 46 cycles into the hblank period.
        status |= hblank_flag;
        // TODO: mGBA triggers the HBlank IRQ and DMAs at this point instead of at 960 cycles, but higan does not.
        // Need to do more research on the correct timing.
    } else if (updated_cycles >= 1232) {
        updated_cycles -= 1232;

        status &= ~hblank_flag;

        if (++vcount == 160) {
            // Begin vblank.
            status |= vblank_flag;

            if (status & vblank_irq) {
                core.mem->RequestInterrupt(Interrupt::VBlank);
            }

            for (auto& dma : core.dma) {
                dma.Trigger(Dma::VBlank);
            }

            core.SwapBuffers(back_buffer);
        } else if (vcount == 227) {
            // Vblank flag is unset one scanline before vblank ends.
            status &= ~vblank_flag;
        } else if (vcount == 228) {
            // Start new frame.
            vcount = 0;
        }

        if (vcount == VTrigger()) {
            status |= vcount_flag;

            if (status & vcount_irq) {
                core.mem->RequestInterrupt(Interrupt::VCount);
            }
        } else {
            status &= ~vcount_flag;
        }
    }

    scanline_cycles = updated_cycles;
}

void Lcd::DrawScanline() {
    if (ForcedBlank() || BgMode() == 2 || BgMode() == 5) {
        // Other BG modes unimplemented for now.
        std::fill_n(back_buffer.begin() + vcount * h_pixels, h_pixels, 0x7FFF);
        return;
    }

    if (BgMode() == 0 || BgMode() == 1) {
        for (int b = 0; b < 4; ++b) {
            if (!BgEnabled(b)) {
                continue;
            }

            if (BgMode() == 1 && b > 1) {
                // Affine backgrounds are not implemented yet, so only draw the first two backgrounds in mode 1.
                break;
            }

            bgs[b].GetRowMapInfo();
            bgs[b].GetTileData();
            bgs[b].DrawScanline();
        }

        // Compose the background scanlines together based on their priorities. 0 is the highest priority value, and 3
        // is the lowest. If multiple backgrounds have the same priority value, then the lower-numbered background has
        // higher priority.
        std::array<const Bg*, 4> sorted_bgs{{&bgs[0], &bgs[1], &bgs[2], &bgs[3]}};
        std::stable_sort(sorted_bgs.begin(), sorted_bgs.end(), [](const auto& bg1, const auto& bg2) {
                return bg1->Priority() < bg2->Priority();
        });

        // The first palette entry is the backdrop colour.
        std::fill_n(back_buffer.begin() + vcount * h_pixels, h_pixels, pram[0] & 0x7FFF);
        for (int b = 3; b >= 0; --b) {
            if (!BgEnabled(b)) {
                continue;
            }

            for (int i = 0; i < h_pixels; ++i) {
                if ((sorted_bgs[b]->scanline[i] & alpha_bit) == 0) {
                    back_buffer[vcount * h_pixels + i] = sorted_bgs[b]->scanline[i];
                }
            }
        }
    } else if (BgMode() == 3) {
        for (int i = 0; i < h_pixels; ++i) {
            back_buffer[vcount * h_pixels + i] = vram[vcount * h_pixels + i];
        }
    } else if (BgMode() == 4) {
        const int base_addr = vcount * h_pixels + (DisplayFrame1() ? 0xA000 : 0);
        for (int i = 0; i < h_pixels; ++i) {
            // The lower byte is the palette index for even pixels, and the upper byte is for odd pixels.
            const int odd_shift = 8 * (i & 0x1);
            u8 palette_entry = vram[(base_addr + i) / 2] >> odd_shift;
            back_buffer[vcount * h_pixels + i] = pram[palette_entry];
        }
    }
}

} // End namespace Gba
