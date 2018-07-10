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
#include <stdexcept>

#include "gba/lcd/Lcd.h"
#include "gba/lcd/Bg.h"
#include "gba/core/Core.h"
#include "gba/core/Enums.h"
#include "gba/memory/Memory.h"
#include "gba/hardware/Dma.h"

namespace Gba {

Lcd::Lcd(const std::vector<u16>& _pram, const std::vector<u16>& _vram, const std::vector<u32>& _oam, Core& _core)
        : bgs{{0, *this}, {1, *this}, {2, *this}, {3, *this}}
        , windows(2)
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

            for (int b = 2; b < 4; ++b) {
                bgs[b].LatchReferencePointX();
                bgs[b].LatchReferencePointY();
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
    } else if (scanline_cycles < 1006 && updated_cycles >= 1006) {
        // The hblank flag isn't set until 46 cycles into the hblank period.
        // This is placed last so it can be skipped while in halt mode.
        status |= hblank_flag;
        // TODO: mGBA triggers the HBlank IRQ and DMAs at this point instead of at 960 cycles, but higan does not.
        // Need to do more research on the correct timing.
    }

    scanline_cycles = updated_cycles;
}

void Lcd::WriteControl(const u16 data, const u16 mask) {
    const std::array<bool, 4> was_disabled{{!bgs[0].Enabled(), !bgs[1].Enabled(),
                                            !bgs[2].Enabled(), !bgs[3].Enabled()}};
    control.Write(data, mask);

    for (int i = 0; i < 4; ++i) {
        if (was_disabled[i] && bgs[i].Enabled()) {
            if (vcount < 160 && !ForcedBlank()) {
                bgs[i].enable_delay = 3;
            } else {
                bgs[i].enable_delay = 0;
            }
        }
    }
}

int Lcd::NextEvent() const {
    if (scanline_cycles < 960) {
        return 960 - scanline_cycles;
    } else {
        return 1232 - scanline_cycles;
    }
}

void Lcd::DrawScanline() {
    if (ForcedBlank()) {
        // Scanlines are drawn white when forced blank is enabled.
        std::fill_n(back_buffer.begin() + vcount * h_pixels, h_pixels, 0x7FFF);
        return;
    }

    if (ObjEnabled()) {
        ReadOam();
        DrawSprites();
    }

    if (bg_dirty) {
        for (auto& bg : bgs) {
            bg.dirty = true;
        }

        bg_dirty = false;
    }

    std::array<std::vector<const Bg*>, 4> priorities;

    if (BgMode() == 0) {
        for (int b = 0; b < 4; ++b) {
            if (bgs[b].Enabled()) {
                bgs[b].DrawRegularScanline();
            }
        }

        // Organize the backgrounds by their priorities. 0 is the highest priority value, and 3 is the lowest.
        // If multiple backgrounds have the same priority value, the lower-numbered background has higher priority.
        for (int b = 3; b >= 0; --b) {
            if (bgs[b].Enabled()) {
                priorities[bgs[b].Priority()].push_back(&bgs[b]);
            }
        }
    } else if (BgMode() == 1) {
        for (int b = 0; b < 2; ++b) {
            if (bgs[b].Enabled()) {
                bgs[b].DrawRegularScanline();
            }
        }

        if (bgs[2].Enabled()) {
            bgs[2].DrawAffineScanline();
        }

        for (int b = 2; b >= 0; --b) {
            if (bgs[b].Enabled()) {
                priorities[bgs[b].Priority()].push_back(&bgs[b]);
            }
        }
    } else if (BgMode() == 2) {
        for (int b = 2; b < 4; ++b) {
            if (bgs[b].Enabled()) {
                bgs[b].DrawAffineScanline();
            }
        }

        for (int b = 3; b >= 2; --b) {
            if (bgs[b].Enabled()) {
                priorities[bgs[b].Priority()].push_back(&bgs[b]);
            }
        }
    } else if (BgMode() == 3 || BgMode() == 4 || BgMode() == 5) {
        // Bitmap modes.
        if (bgs[2].Enabled()) {
            bgs[2].DrawBitmapScanline(BgMode(), DisplayFrame1() ? 0xA000 : 0);
            priorities[0].push_back(&bgs[2]);
        }
    } else {
        // It probably just doesn't draw any background in this case, but if this ever happens I'd like to know.
        throw std::runtime_error("Invalid BG mode.");
    }

    // The first palette entry is the backdrop colour.
    std::fill_n(back_buffer.begin() + vcount * h_pixels, h_pixels, pram[0] & 0x7FFF);

    std::array<PixelInfo, 240> pixel_info{};
    if (IsSecondTarget(5)) {
        for (auto& pixel : pixel_info) {
            pixel.highest_second_target = 5;
        }
    }

    // If alpha blending is enabled, or if semi-transparent sprites are present, calculate the highest first target
    // layer and second target layer for each pixel.
    if (BlendMode() == Effect::AlphaBlend || semi_transparent_used) {
        // Inspect each enabled background, starting with the lowest priority level.
        for (int p = 3; p >= 0; --p) {
            for (const auto& bg : priorities[p]) {
                for (int i = 0; i < h_pixels; ++i) {
                    if ((bg->scanline[i] & alpha_bit) == 0) {
                        if (IsFirstTarget(bg->id)) {
                            pixel_info[i].highest_first_target = bg->id;
                        } else if (IsSecondTarget(bg->id)) {
                            pixel_info[i].highest_second_target = bg->id;
                        }
                    }
                }
            }

            if (ObjEnabled() && sprite_scanline_used[p]) {
                // There is only one sprite layer, even though each sprite can have varying priorities. When
                // calculating blending effects, the GBA only considers the highest priority sprite on each pixel.
                for (int i = 0; i < h_pixels; ++i) {
                    if ((sprite_scanlines[p][i] & alpha_bit) == 0) {
                        if (IsFirstTarget(4) || (sprite_flags[i] & semi_transparent_flag)) {
                            pixel_info[i].highest_first_target = 4;
                        } else if (IsSecondTarget(4)) {
                            pixel_info[i].highest_second_target = 4;
                        }
                    }
                }
            }
        }
    }

    for (int w = 0; w < 2; ++w) {
        windows[w].IsOnThisScanline(WinEnabled(w), vcount);
    }

    // Draw the scanlines from each enabled background, starting with the lowest priority level.
    std::array<int, 3> channels;
    for (int p = 3; p >= 0; --p) {
        for (const auto& bg : priorities[p]) {
            for (int i = 0; i < h_pixels; ++i) {
                if ((bg->scanline[i] & alpha_bit) == 0 && IsWithinWindow(bg->id, i)) {
                    auto& buffer_pixel = back_buffer[vcount * h_pixels + i];

                    if (BlendMode() == Effect::AlphaBlend && pixel_info[i].HighestTargetLayers(bg->id)
                            && IsWithinWindow(5, i)) {
                        for (int j = 0; j < 3; ++j) {
                            int channel_target1 = (bg->scanline[i] >> (5 * j)) & 0x1F;
                            int channel_target2 = (buffer_pixel >> (5 * j)) & 0x1F;

                            channels[j] = Blend(channel_target1, channel_target2);
                        }

                        buffer_pixel = (channels[2] << 10) | (channels[1] << 5) | channels[0];
                    } else {
                        buffer_pixel = bg->scanline[i];
                    }

                    pixel_info[i].layer = bg->id;
                }
            }
        }

        if (ObjEnabled() && sprite_scanline_used[p]) {
            // Draw sprites of the same priority level.
            for (int i = 0; i < h_pixels; ++i) {
                if ((sprite_scanlines[p][i] & alpha_bit) == 0 && IsWithinWindow(4, i)) {
                    auto& buffer_pixel = back_buffer[vcount * h_pixels + i];

                    if ((BlendMode() == Effect::AlphaBlend || (sprite_flags[i] & semi_transparent_flag))
                            && pixel_info[i].HighestTargetLayers(4)
                            && IsWithinWindow(5, i)) {
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
                        sprite_flags[i] &= ~semi_transparent_flag;
                    }

                    pixel_info[i].layer = 4;
                }
            }
        }
    }

    if (BlendMode() == Effect::Brighten || BlendMode() == Effect::Darken) {
        for (int i = 0; i < h_pixels; ++i) {
            if (IsFirstTarget(pixel_info[i].layer)
                    && !(pixel_info[i].layer == 4 && (sprite_flags[i] & semi_transparent_flag))
                    && IsWithinWindow(5, i)) {
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

    for (auto& bg : bgs) {
        if (bg.enable_delay > 0) {
            bg.enable_delay -= 1;
        }
    }
}

bool Lcd::IsWithinWindow(int layer_id, int x) const {
    if (NoWinEnabled()) {
        return true;
    }

    if (windows[0].Contains(x)) {
        return winin & (1 << layer_id);
    } else if (windows[1].Contains(x)) {
        return (winin >> 8) & (1 << layer_id);
    } else if (ObjWinEnabled() && (sprite_flags[x] & obj_window_flag)) {
        return (winout >> 8) & (1 << layer_id);
    } else {
        return winout & (1 << layer_id);
    }
}

void Lcd::ReadOam() {
    // Only update our sprite objects if OAM has been written to.
    if (oam_dirty) {
        sprites.clear();

        // Get all enabled, potentially onscreen sprites.
        for (std::size_t i = 0; i < oam.size(); i += 2) {
            if (!Sprite::Disabled(oam[i])) {
                int y_pos = oam[i] & 0xFF;
                int pixel_height = Sprite::Height(oam[i]);
                if (y_pos + pixel_height > 0xFF) {
                    y_pos -= 0x100;
                }
                if (y_pos < 160) {
                    sprites.emplace_back(oam[i], oam[i + 1]);
                }
            }
        }

        oam_dirty = false;
    }

    // The number of sprites that can be drawn on one scanline depends on the number of cycles each sprite takes
    // to render. The maximum rendering time is reduced if HBlank Interval Free is set.
    const int max_render_cycles = HBlankFree() ? 954 : 1210;
    int render_cycles_needed = 0;
    for (auto& sprite : sprites) {
        sprite.drawn = false;
        if (sprite.y_pos <= vcount && vcount < sprite.y_pos + sprite.pixel_height) {
            // All sprites, including offscreen ones, contribute to rendering time.
            if (sprite.affine) {
                render_cycles_needed += sprite.pixel_width * 2 + 10;
            } else {
                render_cycles_needed += sprite.pixel_width;
            }

            // Don't draw any more sprites once we run out of rendering cycles.
            if (render_cycles_needed > max_render_cycles) {
                continue;
            }

            // Only onscreen sprites will actually be drawn.
            // In bitmap BG modes, attempts to use sprite tiles < 512 are not displayed.
            if (sprite.x_pos < h_pixels && sprite.x_pos + sprite.pixel_width >= 0
                    && (BgMode() < 3 || sprite.tile_num >= 512)) {
                sprite.drawn = true;
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

    if (semi_transparent_used || obj_window_used) {
        std::fill(sprite_flags.begin(), sprite_flags.end(), 0);
        semi_transparent_used = false;
        obj_window_used = false;
    }

    for (int s = sprites.size() - 1; s >= 0; --s) {
        const auto& sprite = sprites[s];

        if (!sprite.drawn) {
            continue;
        }

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

    if (sprite.mosaic && vcount % MosaicObjV() != 0) {
        tile_row = (vcount - vcount % MosaicObjV() - sprite.y_pos) / 8;
        pixel_row = (vcount - vcount % MosaicObjV() - sprite.y_pos) % 8;

        if (tile_row < 0 || pixel_row < 0) {
            return;
        }
    }

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
        int tile_addr = sprite.tile_base_addr + tile_index * sprite.tile_bytes;
        if (ObjMapping2D()) {
            const int h = tile_index / sprite.tile_width;
            tile_addr += h * sprite.tile_bytes * ((sprite.single_palette ? 16 : 32) - sprite.tile_width);
        }
        tile_index += tile_direction;

        std::array<u16, 8> pixel_colours = GetTilePixels(tile_addr, sprite.single_palette, sprite.h_flip, pixel_row,
                                                         sprite.palette, 256);

        // The first and last tiles may be partially scrolled off-screen.
        const int end_offset = std::min(h_pixels - scanline_index, 8);
        for (int i = start_offset; i < end_offset; ++i) {
            if (sprite.mosaic && scanline_index % MosaicObjH() != 0) {
                pixel_colours[i] = sprite_scanlines[sprite.priority][scanline_index - (scanline_index % MosaicObjH())];
            }

            if ((pixel_colours[i] & alpha_bit) == 0) {
                if (ObjWinEnabled() && sprite.mode == Sprite::Mode::ObjWindow) {
                    sprite_flags[scanline_index] |= obj_window_flag;
                    obj_window_used = true;
                } else {
                    sprite_scanlines[sprite.priority][scanline_index] = pixel_colours[i];

                    // Erase sprite pixels at a lower priority than this one, since we only have one object plane.
                    for (int j = sprite.priority + 1; j < 4; ++j) {
                        sprite_scanlines[j][scanline_index] |= alpha_bit;
                    }

                    if (sprite.mode == Sprite::Mode::SemiTransparent) {
                        sprite_flags[scanline_index] |= semi_transparent_flag;
                        semi_transparent_used = true;
                    }
                }
            }

            scanline_index += 1;
        }
        start_offset = 0;
    }
}

void Lcd::DrawAffineSprite(const Sprite& sprite) {
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
        const int sprite_x = scanline_index - sprite_centre_x;

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

        const int tile_row = tex_y / 8;
        const int pixel_row = tex_y % 8;
        const int tile_index = tile_row * sprite.tile_width + tex_x / 8;

        int tile_addr = sprite.tile_base_addr + tile_index * sprite.tile_bytes;
        if (ObjMapping2D()) {
            const int h = tile_index / sprite.tile_width;
            tile_addr += h * sprite.tile_bytes * ((sprite.single_palette ? 16 : 32) - sprite.tile_width);
        }

        if (sprite.single_palette) {
            // Each tile byte specifies the 8-bit palette index for a pixel.
            const int pixel_addr = tile_addr + pixel_row * 8 + tex_x % 8;
            const int hi_shift = 8 * (pixel_addr & 0x1);

            const u8 palette_entry = (vram[pixel_addr / 2] >> hi_shift) & 0xFF;
            if (palette_entry != 0) {
                // Palette entry 0 is transparent.
                if (ObjWinEnabled() && sprite.mode == Sprite::Mode::ObjWindow) {
                    sprite_flags[scanline_index] |= obj_window_flag;
                    obj_window_used = true;
                } else {
                    sprite_scanlines[sprite.priority][scanline_index] = pram[256 + palette_entry] & 0x7FFF;

                    // Erase sprite pixels at a lower priority than this one, since we only have one object plane.
                    for (int j = sprite.priority + 1; j < 4; ++j) {
                        sprite_scanlines[j][scanline_index] |= alpha_bit;
                    }

                    if (sprite.mode == Sprite::Mode::SemiTransparent) {
                        sprite_flags[scanline_index] |= semi_transparent_flag;
                        semi_transparent_used = true;
                    }
                }
            }
        } else {
            const int pixel_addr = tile_addr + pixel_row * 4 + (tex_x % 8) / 2;
            const int hi_shift = 8 * (pixel_addr & 0x1);

            // The lower 4 bits are the palette index for even pixels, and the upper 4 bits are for odd pixels.
            const int odd_shift = 4 * ((tex_x % 8) & 0x1);
            const u8 palette_entry = (vram[pixel_addr / 2] >> (hi_shift + odd_shift)) & 0xF;
            if (palette_entry != 0) {
                // Palette entry 0 is transparent.
                if (ObjWinEnabled() && sprite.mode == Sprite::Mode::ObjWindow) {
                    sprite_flags[scanline_index] |= obj_window_flag;
                    obj_window_used = true;
                } else {
                    sprite_scanlines[sprite.priority][scanline_index] = pram[256 + sprite.palette * 16 + palette_entry] & 0x7FFF;

                    // Erase sprite pixels at a lower priority than this one, since we only have one object plane.
                    for (int j = sprite.priority + 1; j < 4; ++j) {
                        sprite_scanlines[j][scanline_index] |= alpha_bit;
                    }

                    if (sprite.mode == Sprite::Mode::SemiTransparent) {
                        sprite_flags[scanline_index] |= semi_transparent_flag;
                        semi_transparent_used = true;
                    }
                }
            }
        }

        scanline_index += 1;
    }
}

std::array<u16, 8> Lcd::GetTilePixels(int tile_addr, bool single_palette, bool h_flip,
                                      int pixel_row, int palette, int base) const {
    std::array<u16, 8> pixel_colours;

    if (single_palette) {
        // Each tile byte specifies the 8-bit palette index for a pixel.
        // We read two bytes at a time from VRAM.
        for (int i = 0; i < 4; ++i) {
            const int pixel_addr = tile_addr + pixel_row * 8 + i * 2;
            const u16 vram_entry = vram[pixel_addr / 2];

            for (int j = 0; j < 2; ++j) {
                const u8 palette_entry = (vram_entry >> (j * 8)) & 0xFF;
                if (palette_entry == 0) {
                    // Palette entry 0 is transparent.
                    pixel_colours[i * 2 + j] = alpha_bit;
                } else {
                    pixel_colours[i * 2 + j] = pram[base + palette_entry] & 0x7FFF;
                }
            }
        }
    } else {
        // Each tile byte specifies the 4-bit palette indices for two pixels.
        // The lower 4 bits are the palette index for even pixels, and the upper 4 bits are for odd pixels.
        // We read two bytes at a time from VRAM.
        for (int i = 0; i < 2; ++i) {
            const int pixel_addr = tile_addr + pixel_row * 4 + i * 2;
            const u16 vram_entry = vram[pixel_addr / 2];

            for (int j = 0; j < 4; ++j) {
                const u8 palette_entry = (vram_entry >> (j * 4)) & 0xF;
                if (palette_entry == 0) {
                    // Palette entry 0 is transparent.
                    pixel_colours[i * 4 + j] = alpha_bit;
                } else {
                    pixel_colours[i * 4 + j] = pram[base + palette * 16 + palette_entry] & 0x7FFF;
                }
            }
        }
    }

    if (h_flip) {
        std::reverse(pixel_colours.begin(), pixel_colours.end());
    }

    return pixel_colours;
}

} // End namespace Gba
