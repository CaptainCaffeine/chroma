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

    if (ObjEnabled()) {
        ReadOam();
        GetTileData();
        DrawSprites();
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

        // Organize the backgrounds by their priorities. 0 is the highest priority value, and 3 is the lowest.
        // If multiple backgrounds have the same priority value, the lower-numbered background has higher priority.
        std::array<std::vector<const Bg*>, 4> priorities;
        for (int b = 3; b >= 0; --b) {
            if (BgEnabled(b)) {
                priorities[bgs[b].Priority()].push_back(&bgs[b]);
            }
        }

        // The first palette entry is the backdrop colour.
        std::fill_n(back_buffer.begin() + vcount * h_pixels, h_pixels, pram[0] & 0x7FFF);

        // Draw the scanlines from each enabled background, starting with the lowest priority level.
        for (int p = 3; p >= 0; --p) {
            for (const auto& bg : priorities[p]) {
                for (int i = 0; i < h_pixels; ++i) {
                    if ((bg->scanline[i] & alpha_bit) == 0) {
                        back_buffer[vcount * h_pixels + i] = bg->scanline[i];
                    }
                }
            }

            if (ObjEnabled()) {
                // Draw sprites of the same priority level.
                for (int i = 0; i < h_pixels; ++i) {
                    if ((sprite_scanlines[p][i] & alpha_bit) == 0) {
                        back_buffer[vcount * h_pixels + i] = sprite_scanlines[p][i];
                    }
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

void Lcd::ReadOam() {
    sprites.clear();

    // The number of sprites that can be drawn on one scanline depends on the number of cycles each sprite takes
    // to render. The maximum rendering time is reduced if HBlank Interval Free is set.
    const int max_render_cycles = HBlankFree() ? 954 : 1210;
    int render_cycles_needed = 0;
    for (std::size_t i = 0; i < oam.size(); i += 2) {
        Sprite sprite{oam[i], oam[i + 1]};
        if (sprite.y_pos <= vcount && vcount < sprite.y_pos + sprite.pixel_height && !sprite.disable) {
            // All sprites, including offscreen ones, contribute to rendering time.
            if (sprite.affine) {
                render_cycles_needed += sprite.pixel_width * 2 + 10;
            } else {
                render_cycles_needed += sprite.pixel_width;
            }

            // Don't draw any more sprites once we run out of rendering cycles.
            if (render_cycles_needed > max_render_cycles) {
                break;
            }

            // Only onscreen sprites will actually be drawn.
            if (sprite.x_pos < h_pixels && sprite.x_pos + sprite.pixel_width >= 0) {
                sprites.push_back(sprite);
            }
        }
    }
}

void Lcd::GetTileData() {
    // Get tile data. Each tile is 32 bytes in 16 palette mode, and 64 bytes in single palette mode.
    for (auto& sprite : sprites) {
        int tile_bytes = 32;
        if (sprite.single_palette) {
            tile_bytes = 64;
            sprite.tile_num &= ~0x1;
        }

        if (ObjMapping1D()) {
            for (std::size_t t = 0; t < sprite.tiles.size(); ++t) {
                const int tile_addr = (sprite_tile_base + sprite.tile_num * 32 + t * tile_bytes) / 2;
                for (int i = 0; i < tile_bytes; i += 2) {
                    sprite.tiles[t][i] = vram[tile_addr + i / 2];
                    sprite.tiles[t][i + 1] = vram[tile_addr + i / 2] >> 8;
                }
            }
        } else {
            for (int h = 0; h < sprite.tile_height; ++h) {
                for (int w = 0; w < sprite.tile_width; ++w) {
                    const int tile_addr = (sprite_tile_base + sprite.tile_num * 32 + h * 32 * 32 + w * tile_bytes) / 2;
                    const int tile_index = h * sprite.tile_width + w;
                    for (int i = 0; i < tile_bytes; i += 2) {
                        sprite.tiles[tile_index][i] = vram[tile_addr + i / 2];
                        sprite.tiles[tile_index][i + 1] = vram[tile_addr + i / 2] >> 8;
                    }
                }
            }
        }
    }
}

void Lcd::DrawSprites() {
    // Clear each sprite scanline.
    for (auto& scanline : sprite_scanlines) {
        std::fill(scanline.begin(), scanline.end(), 0x8000);
    }

    for (int s = sprites.size() - 1; s >= 0; --s) {
        const auto& sprite = sprites[s];

        int tile_row = (vcount - sprite.y_pos) / 8;
        int pixel_row = (vcount - sprite.y_pos) % 8;
        if (sprite.v_flip) {
            tile_row = (sprite.tile_height - 1) - tile_row;
            pixel_row = 7 - pixel_row;
        }

        const int first_tile = tile_row * sprite.tile_width;
        const int last_tile = (tile_row + 1) * sprite.tile_width - 1;

        int start_offset = 0;
        int scanline_index = sprite.x_pos;
        if (sprite.x_pos < 0) {
            start_offset = -sprite.x_pos % 8;
            scanline_index = 0;
        }

        // If the sprite is horizontally flipped, we start drawing from the rightmost tile.
        int tile_index = first_tile;
        int tile_direction = 1;
        if (sprite.h_flip) {
            tile_index = last_tile;
            tile_direction = -1;
        }

        if (sprite.x_pos < 0) {
            // Start drawing at the first onscreen tile.
            tile_index += (-sprite.x_pos / 8) * tile_direction;
        }

        while (scanline_index < h_pixels && tile_index <= last_tile && tile_index >= first_tile) {
            auto& tile = sprite.tiles[tile_index];
            tile_index += tile_direction;

            std::array<u16, 8> pixel_colours = GetTilePixels(tile, sprite.single_palette, pixel_row, sprite.palette,
                                                             256);

            if (sprite.h_flip) {
                std::reverse(pixel_colours.begin(), pixel_colours.end());
            }

            // The first and last tiles may be partially scrolled off-screen.
            const int end_offset = std::min(h_pixels - scanline_index, 8);
            for (int i = start_offset; i < end_offset; ++i) {
                if ((pixel_colours[i] & alpha_bit) == 0) {
                    sprite_scanlines[sprite.priority][scanline_index] = pixel_colours[i];
                }

                scanline_index += 1;
            }
            start_offset = 0;
        }
    }
}

std::array<u16, 8> Lcd::GetTilePixels(const Tile& tile, bool single_palette, int pixel_row, int palette, int base) {
    std::array<u16, 8> pixel_colours;

    if (single_palette) {
        // Each tile byte specifies the 8-bit palette index for a pixel.
        for (int i = 0; i < 8; ++i) {
            u8 palette_entry = tile[pixel_row * 8 + i];
            pixel_colours[i] = pram[base + palette_entry];

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
            u8 palette_entry = (tile[pixel_row * 4 + i / 2] >> odd_shift) & 0xF;
            pixel_colours[i] = pram[base + palette * 16 + palette_entry];

            if (palette_entry == 0) {
                // Palette entry 0 is transparent.
                pixel_colours[i] |= alpha_bit;
            }
        }
    }

    return pixel_colours;
}

} // End namespace Gba
