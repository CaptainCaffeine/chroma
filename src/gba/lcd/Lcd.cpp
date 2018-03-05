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
        : bgs{{0, *this}, {1, *this}, {2, *this}, {3, *this}}
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

    std::array<std::vector<const Bg*>, 4> priorities;

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
        for (int b = 3; b >= 0; --b) {
            if (BgEnabled(b)) {
                priorities[bgs[b].Priority()].push_back(&bgs[b]);
            }
        }
    } else if (BgMode() == 3) {
        for (int i = 0; i < h_pixels; ++i) {
            bgs[2].scanline[i] = vram[vcount * h_pixels + i] & 0x7FFF;
        }

        priorities[0].push_back(&bgs[2]);
    } else if (BgMode() == 4) {
        const int base_addr = vcount * h_pixels + (DisplayFrame1() ? 0xA000 : 0);
        for (int i = 0; i < h_pixels; ++i) {
            // The lower byte is the palette index for even pixels, and the upper byte is for odd pixels.
            const int odd_shift = 8 * (i & 0x1);
            u8 palette_entry = vram[(base_addr + i) / 2] >> odd_shift;
            bgs[2].scanline[i] = pram[palette_entry];
        }

        priorities[0].push_back(&bgs[2]);
    }

    // The first palette entry is the backdrop colour.
    std::fill_n(back_buffer.begin() + vcount * h_pixels, h_pixels, pram[0] & 0x7FFF);
    std::vector<int> pixel_layer(240, 5);

    // The target vectors are initialized with non-existent layer 6.
    std::vector<int> highest_second_target(240, IsSecondTarget(5) ? 5 : 6);
    std::vector<int> highest_first_target(240, 6);

    // If alpha blending is enabled, or if semi-transparent sprites are present, calculate the highest first target
    // layer and second target layer for each pixel.
    if (BlendMode() == Effect::AlphaBlend || semi_transparent_used) {
        // Inspect each enabled background, starting with the lowest priority level.
        for (int p = 3; p >= 0; --p) {
            for (const auto& bg : priorities[p]) {
                for (int i = 0; i < h_pixels; ++i) {
                    if ((bg->scanline[i] & alpha_bit) == 0) {
                        if (IsSecondTarget(bg->id)) {
                            highest_second_target[i] = bg->id;
                        }
                        if (IsFirstTarget(bg->id)) {
                            highest_first_target[i] = bg->id;
                        }
                    }
                }
            }

            if (ObjEnabled() && sprite_scanline_used[p]) {
                // There is only one sprite layer, even though each sprite can have varying priorities. When
                // calculating blending effects, the GBA only considers the highest priority sprite on each pixel.
                for (int i = 0; i < h_pixels; ++i) {
                    if ((sprite_scanlines[p][i] & alpha_bit) == 0) {
                        if (IsSecondTarget(4)) {
                            highest_second_target[i] = 4;
                        }
                        if (IsFirstTarget(4) || semi_transparent[i]) {
                            highest_first_target[i] = 4;
                        }
                    }
                }
            }
        }
    }

    auto HighestTargetLayers = [&highest_first_target, &highest_second_target, &pixel_layer](int layer, int i) {
        return layer == highest_first_target[i] && pixel_layer[i] == highest_second_target[i];
    };

    // Draw the scanlines from each enabled background, starting with the lowest priority level.
    std::array<int, 3> channels;
    for (int p = 3; p >= 0; --p) {
        for (const auto& bg : priorities[p]) {
            for (int i = 0; i < h_pixels; ++i) {
                if ((bg->scanline[i] & alpha_bit) == 0) {
                    auto& buffer_pixel = back_buffer[vcount * h_pixels + i];

                    if (BlendMode() == Effect::AlphaBlend && HighestTargetLayers(bg->id, i)) {
                        for (int j = 0; j < 3; ++j) {
                            int channel_target1 = (bg->scanline[i] >> (5 * j)) & 0x1F;
                            int channel_target2 = (buffer_pixel >> (5 * j)) & 0x1F;

                            channels[j] = Blend(channel_target1, channel_target2);
                        }

                        buffer_pixel = (channels[2] << 10) | (channels[1] << 5) | channels[0];
                    } else {
                        buffer_pixel = bg->scanline[i];
                    }

                    pixel_layer[i] = bg->id;
                }
            }
        }

        if (ObjEnabled() && sprite_scanline_used[p]) {
            // Draw sprites of the same priority level.
            for (int i = 0; i < h_pixels; ++i) {
                if ((sprite_scanlines[p][i] & alpha_bit) == 0) {
                    auto& buffer_pixel = back_buffer[vcount * h_pixels + i];

                    if ((BlendMode() == Effect::AlphaBlend || semi_transparent[i]) && HighestTargetLayers(4, i)) {
                        for (int j = 0; j < 3; ++j) {
                            int channel_target1 = (sprite_scanlines[p][i] >> (5 * j)) & 0x1F;
                            int channel_target2 = (buffer_pixel >> (5 * j)) & 0x1F;

                            channels[j] = Blend(channel_target1, channel_target2);
                        }

                        buffer_pixel = (channels[2] << 10) | (channels[1] << 5) | channels[0];
                    } else {
                        buffer_pixel = sprite_scanlines[p][i];

                        // If a semi-transparent sprite blends, no other blending effects can occur on this pixel.
                        // So if a sprite pixel doesn't blend, we remove the semi-transparent flag (if present) so
                        // fade effects can be applied later.
                        semi_transparent[i] = false;
                    }

                    pixel_layer[i] = 4;
                }
            }
        }
    }

    if (BlendMode() == Effect::Brighten || BlendMode() == Effect::Darken) {
        for (int i = 0; i < h_pixels; ++i) {
            if (IsFirstTarget(pixel_layer[i]) && !(pixel_layer[i] == 4 && semi_transparent[i])) {
                auto& buffer_pixel = back_buffer[vcount * h_pixels + i];

                if (BlendMode() == Effect::Brighten) {
                    for (int j = 0; j < 3; ++j) {
                        channels[j] = Brighten((buffer_pixel >> (5 * j)) & 0x1F);
                    }
                } else if (BlendMode() == Effect::Darken) {
                    for (int j = 0; j < 3; ++j) {
                        channels[j] = Darken((buffer_pixel >> (5 * j)) & 0x1F);
                    }
                }

                buffer_pixel = (channels[2] << 10) | (channels[1] << 5) | channels[0];
            }
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
        if (!sprite.Disabled() && sprite.y_pos <= vcount && vcount < sprite.y_pos + sprite.pixel_height) {
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
            // In bitmap BG modes, attempts to use sprite tiles < 512 are not displayed.
            if (sprite.x_pos < h_pixels && sprite.x_pos + sprite.pixel_width >= 0
                    && (BgMode() < 3 || sprite.tile_num >= 512)) {
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
                const int tile_addr = sprite_tile_base + sprite.tile_num * 32 + t * tile_bytes;
                for (int i = 0; i < tile_bytes; i += 2) {
                    sprite.tiles[t][i] = vram[(tile_addr + i) / 2];
                    sprite.tiles[t][i + 1] = vram[(tile_addr + i) / 2] >> 8;
                }
            }
        } else {
            for (int h = 0; h < sprite.tile_height; ++h) {
                for (int w = 0; w < sprite.tile_width; ++w) {
                    const int tile_addr = sprite_tile_base + sprite.tile_num * 32 + h * 32 * 32 + w * tile_bytes;
                    const int tile_index = h * sprite.tile_width + w;
                    for (int i = 0; i < tile_bytes; i += 2) {
                        sprite.tiles[tile_index][i] = vram[(tile_addr + i) / 2];
                        sprite.tiles[tile_index][i + 1] = vram[(tile_addr + i) / 2] >> 8;
                    }
                }
            }
        }
    }
}

void Lcd::DrawSprites() {
    // Only clear used sprite scanlines.
    for (int s = 0; s < 4; ++s) {
        if (sprite_scanline_used[s]) {
            std::fill(sprite_scanlines[s].begin(), sprite_scanlines[s].end(), 0x8000);
            sprite_scanline_used[s] = false;
        }
    }

    if (semi_transparent_used) {
        std::fill(semi_transparent.begin(), semi_transparent.end(), false);
        semi_transparent_used = false;
    }


    for (int s = sprites.size() - 1; s >= 0; --s) {
        const auto& sprite = sprites[s];

        sprite_scanline_used[sprite.priority] = true;

        if (sprite.affine) {
            DrawAffineSprite(sprite);
        } else {
            DrawRegularSprite(sprite);
        }
    }
}

void Lcd::DrawRegularSprite(const Sprite& sprite) {
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

        std::array<u16, 8> pixel_colours = GetTilePixels(tile, sprite.single_palette, pixel_row, sprite.palette, 256);

        if (sprite.h_flip) {
            std::reverse(pixel_colours.begin(), pixel_colours.end());
        }

        // The first and last tiles may be partially scrolled off-screen.
        const int end_offset = std::min(h_pixels - scanline_index, 8);
        for (int i = start_offset; i < end_offset; ++i) {
            if ((pixel_colours[i] & alpha_bit) == 0) {
                sprite_scanlines[sprite.priority][scanline_index] = pixel_colours[i];

                // Erase sprite pixels at a lower priority than this one, since we only have one object plane.
                for (int j = sprite.priority + 1; j < 4; ++j) {
                    sprite_scanlines[j][scanline_index] |= alpha_bit;
                }

                semi_transparent[scanline_index] = sprite.mode == Sprite::Mode::SemiTransparent;
                semi_transparent_used = semi_transparent_used || sprite.mode == Sprite::Mode::SemiTransparent;
            }

            scanline_index += 1;
        }
        start_offset = 0;
    }
}

void Lcd::DrawAffineSprite(const Sprite& sprite) {
    sprite_scanline_used[sprite.priority] = true;

    const int tex_centre_x = sprite.pixel_width / 2;
    const int tex_centre_y = sprite.pixel_height / 2;

    const int sprite_centre_x = tex_centre_x + sprite.x_pos;
    const int sprite_centre_y = tex_centre_y + sprite.y_pos;

    int scanline_index = std::max(sprite.x_pos, 0);
    const int last_sprite_pixel = std::min(sprite.x_pos + sprite.pixel_width, 240);

    // Affine parameters.
    const int pa = static_cast<s32>(oam[sprite.affine_select * 8 + 1]) >> 16;
    const int pb = static_cast<s32>(oam[sprite.affine_select * 8 + 3]) >> 16;
    const int pc = static_cast<s32>(oam[sprite.affine_select * 8 + 5]) >> 16;
    const int pd = static_cast<s32>(oam[sprite.affine_select * 8 + 7]) >> 16;

    const int sprite_y = vcount - sprite_centre_y;
    const int pb_sprite_y = pb * sprite_y;
    const int pd_sprite_y = pd * sprite_y;

    while (scanline_index < last_sprite_pixel) {
        int sprite_x = scanline_index - sprite_centre_x;

        int tex_x = ((pa * sprite_x + pb_sprite_y) >> 8) + tex_centre_x;
        int tex_y = ((pc * sprite_x + pd_sprite_y) >> 8) + tex_centre_y;

        if (sprite.double_size) {
            tex_x -= sprite.pixel_width / 4;
            tex_y -= sprite.pixel_height / 4;
        }

        if (tex_x >= sprite.pixel_width || tex_x < 0 || tex_y >= sprite.pixel_height || tex_y < 0) {
            scanline_index += 1;
            continue;
        } else if (sprite.double_size && (tex_x >= sprite.pixel_width / 2 || tex_y >= sprite.pixel_height / 2)) {
            scanline_index += 1;
            continue;
        }

        int tile_row = tex_y / 8;
        int pixel_row = tex_y % 8;
        int tile_index = tile_row * sprite.tile_width + tex_x / 8;

        if (sprite.single_palette) {
            // Each tile byte specifies the 8-bit palette index for a pixel.
            const int pixel_index = pixel_row * 8 + tex_x % 8;
            u8 palette_entry = sprite.tiles[tile_index][pixel_index];
            if (palette_entry != 0) {
                // Palette entry 0 is transparent.
                sprite_scanlines[sprite.priority][scanline_index] = pram[256 + palette_entry];

                // Erase sprite pixels at a lower priority than this one, since we only have one object plane.
                for (int j = sprite.priority + 1; j < 4; ++j) {
                    sprite_scanlines[j][scanline_index] |= alpha_bit;
                }

                semi_transparent[scanline_index] = sprite.mode == Sprite::Mode::SemiTransparent;
                semi_transparent_used = semi_transparent_used || sprite.mode == Sprite::Mode::SemiTransparent;
            }
        } else {
            // The lower 4 bits are the palette index for even pixels, and the upper 4 bits are for odd pixels.
            const int odd_shift = 4 * ((tex_x % 8) & 0x1);
            const int pixel_index = pixel_row * 4 + (tex_x % 8) / 2;
            u8 palette_entry = (sprite.tiles[tile_index][pixel_index] >> odd_shift) & 0xF;
            if (palette_entry != 0) {
                // Palette entry 0 is transparent.
                sprite_scanlines[sprite.priority][scanline_index] = pram[256 + sprite.palette * 16 + palette_entry];

                // Erase sprite pixels at a lower priority than this one, since we only have one object plane.
                for (int j = sprite.priority + 1; j < 4; ++j) {
                    sprite_scanlines[j][scanline_index] |= alpha_bit;
                }

                semi_transparent[scanline_index] = sprite.mode == Sprite::Mode::SemiTransparent;
                semi_transparent_used = semi_transparent_used || sprite.mode == Sprite::Mode::SemiTransparent;
            }
        }

        scanline_index += 1;
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
