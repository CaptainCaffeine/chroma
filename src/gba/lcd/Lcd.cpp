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

#include <algorithm>

#include "gba/lcd/Lcd.h"
#include "gba/core/Core.h"
#include "gba/core/Enums.h"
#include "gba/memory/Memory.h"
#include "gba/hardware/Dma.h"

namespace Gba {

Lcd::Lcd(const std::vector<u16>& _pram, const std::vector<u16>& _vram, const std::vector<u32>& _oam, Core& _core)
        : pram(_pram)
        , vram(_vram)
        , oam(_oam)
        , core(_core)
        , back_buffer(h_pixels * v_pixels, 0x7FFF) {}

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
    if (ForcedBlank() || (BgMode() != 0 && BgMode() != 1)) {
        // Other BG modes unimplemented for now.
        std::fill_n(back_buffer.begin() + vcount * h_pixels, h_pixels, 0x7FFF);
        return;
    }

    for (int b = 0; b < 4; ++b) {
        if (!BgEnabled(b)) {
            continue;
        }

        if (BgMode() == 1 && b > 1) {
            // Affine backgrounds are not implemented yet, so only draw the first two backgrounds in mode 1.
            break;
        }

        GetRowMapInfo(bgs[b]);
        GetTileData(bgs[b]);
        DrawBgScanline(bgs[b]);
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
}

void Lcd::GetRowMapInfo(Bg& bg) {
    bg.tiles.clear();

    int row_num = (bg.scroll_y + vcount) / 8;
    if (bg.ScreenSize<int>() < 2) {
        row_num %= 32;
    } else {
        row_num %= 64;
    }

    // Get a row of map entries from the specified screenblock.
    auto ReadRowMap = [this, row_num, &bg](int screenblock) {
        int map_addr = (bg.MapBase() + row_num * 64 + 0x800 * screenblock) / 2;

        for (int i = map_addr; i < map_addr + 32; ++i) {
            bg.tiles.emplace_back(vram[i]);
        }
    };

    // Build the tilemap info for this scanline from the necessary screenblocks.
    switch (bg.ScreenSize<Bg::Regular>()) {
    case Bg::Regular::Size32x32:
        ReadRowMap(0);
        break;
    case Bg::Regular::Size64x32:
        ReadRowMap(0);
        ReadRowMap(1);
        break;
    case Bg::Regular::Size32x64:
        if (row_num < 32) {
            ReadRowMap(0);
        } else {
            ReadRowMap(1);
        }
        break;
    case Bg::Regular::Size64x64:
        if (row_num < 32) {
            ReadRowMap(0);
            ReadRowMap(1);
        } else {
            ReadRowMap(2);
            ReadRowMap(3);
        }
        break;
    default:
        break;
    }
}

void Lcd::GetTileData(Bg& bg) {
    // Get tile data. Each tile is 32 bytes in 16 palette mode, and 64 bytes in single palette mode.
    const int tile_bytes = bg.SinglePalette() ? 64 : 32;
    for (auto& tile : bg.tiles) {
        int tile_addr = (bg.TileBase() + tile.num * tile_bytes) / 2;
        for (int i = 0; i < tile_bytes; i += 2) {
            tile.data[i] = vram[tile_addr + i / 2];
            tile.data[i + 1] = vram[tile_addr + i / 2] >> 8;
        }
    }
}

void Lcd::DrawBgScanline(Bg& bg) {
    const int tile_row = (bg.scroll_y + vcount) % 8;

    const int horizontal_tiles = (bg.ScreenSize<int>() & 0x1) ? 64 : 32;
    int tile_index = (bg.scroll_x / 8) % horizontal_tiles;
    int start_offset = bg.scroll_x % 8;

    int scanline_index = 0;
    while (scanline_index < h_pixels) {
        auto& tile = bg.tiles[tile_index];
        tile_index = (tile_index + 1) % horizontal_tiles;
        const int flip_row = tile.v_flip ? (8 - tile_row) : tile_row;

        std::array<u16, 8> pixel_colours;
        if (bg.SinglePalette()) {
            // Each tile byte specifies the 8-bit palette index for a pixel.
            for (int i = 0; i < 8; ++i) {
                u8 palette_entry = tile.data[flip_row * 8 + i];
                pixel_colours[i] = pram[palette_entry];

                if (palette_entry == 0) {
                    // Palette entry 0 is transparent.
                    pixel_colours[i] |= alpha_bit;
                }
            }
        } else {
            // Each tile byte specifies the 4-bit palette indices for two pixels.
            for (int i = 0; i < 8; ++i) {
                // The lower 4 bits are the palette index for even pixels, and the upper 4 bits are for odd pixels.
                const int odd_shift = 4 * (i & 0x1);
                u8 palette_entry = (tile.data[flip_row * 4 + i / 2] >> odd_shift) & 0xF;
                pixel_colours[i] = pram[tile.palette * 16 + palette_entry];

                if (palette_entry == 0) {
                    // Palette entry 0 is transparent.
                    pixel_colours[i] |= alpha_bit;
                }
            }
        }

        if (tile.h_flip) {
            std::reverse(pixel_colours.begin(), pixel_colours.end());
        }

        // The first and last tiles may be partially scrolled off-screen.
        const int end_offset = std::min(h_pixels - scanline_index, 8);
        for (int i = start_offset; i < end_offset; ++i) {
            bg.scanline[scanline_index++] = pixel_colours[i];
        }
        start_offset = 0;
    }
}

} // End namespace Gba
