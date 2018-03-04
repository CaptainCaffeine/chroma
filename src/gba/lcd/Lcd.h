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

#pragma once

#include <vector>
#include <array>

#include "common/CommonTypes.h"
#include "common/CommonFuncs.h"
#include "gba/memory/IOReg.h"

namespace Gba {

class Core;
class Bg;

using Tile = std::array<u8, 64>;

class Sprite {
public:
    Sprite(u32 attr1, u32 attr2)
            : y_pos(attr1 & 0xFF)
            , affine(attr1 & 0x100)
            , disable(attr1 & 0x200)
            , mode(static_cast<Mode>((attr1 >> 10) & 0x3))
            , mosaic(attr1 & 0x1000)
            , single_palette(attr1 & 0x2000)
            , shape(static_cast<Shape>((attr1 >> 14) & 0x3))
            , x_pos(SignExtend((attr1 >> 16) & 0x1FF, 9))
            , affine_select((attr1 >> 25) & 0x1F)
            , h_flip(attr1 & 0x1000'0000)
            , v_flip(attr1 & 0x2000'0000)
            , size((attr1 >> 30) & 0x3)
            , tile_num(attr2 & 0x3FF)
            , priority((attr2 >> 10) & 0x3)
            , palette((attr2 >> 12) & 0xF)
            , pixel_width(Width(shape, size))
            , pixel_height(Height(shape, size))
            , tile_width(pixel_width / 8)
            , tile_height(pixel_height / 8)
            , tiles(tile_width * tile_height) {

        if (y_pos + pixel_height > 0xFF) {
            y_pos -= 0xFF;
        }
    }

    enum class Mode {Normal          = 0,
                     SemiTransparent = 1,
                     ObjWindow       = 2,
                     Prohibited      = 3};

    enum class Shape {Square     = 0,
                      Horizontal = 1,
                      Vertical   = 2,
                      Prohibited = 3};

    int y_pos;
    bool affine;
    bool disable; // Also double-size in affine mode.
    Mode mode;
    bool mosaic;
    bool single_palette;
    Shape shape;

    int x_pos;
    int affine_select;
    bool h_flip;
    bool v_flip;
    int size;

    int tile_num;
    int priority;
    int palette;

    int pixel_width;
    int pixel_height;
    int tile_width;
    int tile_height;

    std::vector<Tile> tiles;

    static int Height(Shape shape, int size) {
        switch (shape) {
        case Shape::Square:
            // 8, 16, 32, 64
            return 8 << size;
        case Shape::Horizontal:
            // 8, 8, 16, 32
            if (size > 0) {
                size -= 1;
            }
            return 8 << size;
        case Shape::Vertical:
            // 16, 32, 32, 64
            if (size > 1) {
                size -= 1;
            }
            return 16 << size;
        default:
            return 0;
        }
    }

    static int Width(Shape shape, int size) {
        switch (shape) {
        case Shape::Square:
            // 8, 16, 32, 64
            return 8 << size;
        case Shape::Horizontal:
            // 16, 32, 32, 64
            if (size > 1) {
                size -= 1;
            }
            return 16 << size;
        case Shape::Vertical:
            // 8, 8, 16, 32
            if (size > 0) {
                size -= 1;
            }
            return 8 << size;
        default:
            return 0;
        }
    }
};

class Lcd {
public:
    Lcd(const std::vector<u16>& _pram, const std::vector<u16>& _vram, const std::vector<u32>& _oam, Core& _core);
    ~Lcd();

    IOReg control       = {0x0000, 0xFFF7, 0xFFF7};
    IOReg green_swap    = {0x0000, 0x0001, 0x0001};
    IOReg status        = {0x0000, 0xFF3F, 0xFF38};
    IOReg vcount        = {0x0000, 0x00FF, 0x0000};

    IOReg win0_width    = {0x0000, 0x0000, 0xFFFF};
    IOReg win1_width    = {0x0000, 0x0000, 0xFFFF};
    IOReg win0_height   = {0x0000, 0x0000, 0xFFFF};
    IOReg win1_height   = {0x0000, 0x0000, 0xFFFF};
    IOReg winin         = {0x0000, 0x3F3F, 0x3F3F};
    IOReg winout        = {0x0000, 0x3F3F, 0x3F3F};

    IOReg mosaic        = {0x0000, 0x0000, 0xFFFF};
    IOReg blend_control = {0x0000, 0x3FFF, 0x3FFF};
    IOReg blend_alpha   = {0x0000, 0x1F1F, 0x1F1F};
    IOReg blend_fade    = {0x0000, 0x0000, 0x001F};

    std::vector<Bg> bgs;

    const std::vector<u16>& pram;
    const std::vector<u16>& vram;
    const std::vector<u32>& oam;

    static constexpr int h_pixels = 240;
    static constexpr int v_pixels = 160;
    static constexpr u16 alpha_bit = 0x8000;
    static constexpr int sprite_tile_base = 0x1'0000;

    void Update(int cycles);

    std::array<u16, 8> GetTilePixels(const Tile& tile, bool single_palette, int pixel_row, int palette, int base);

private:
    Core& core;

    std::vector<u16> back_buffer;

    int scanline_cycles = 0;

    std::vector<Sprite> sprites;
    std::array<std::array<u16, 240>, 4> sprite_scanlines;
    std::array<bool, 4> sprite_scanline_used{{true, true, true, true}};
    std::array<bool, 240> semi_transparent;
    bool semi_transparent_used = true;

    void DrawScanline();

    void ReadOam();
    void GetTileData();
    void DrawSprites();

    bool IsFirstTarget(int target) const { return (FirstTargets() >> target) & 0x1; }
    bool IsSecondTarget(int target) const { return (SecondTargets() >> target) & 0x1; }

    int Brighten(int t) const { return t + (31.0 - t) * Intensity(); }
    int Darken(int t) const { return t * (1.0 - Intensity()); }
    int Blend(int t1, int t2) const { return std::min(t1 * FirstAlpha() + t2 * SecondAlpha(), 31.0); }

    // Control flags
    int BgMode() const { return control & 0x7; }
    bool DisplayFrame1() const { return control & 0x10; }
    bool HBlankFree() const { return control & 0x20; }
    bool ObjMapping1D() const { return control & 0x40; }
    bool ForcedBlank() const { return control & 0x80; }
    bool BgEnabled(int bg_id) const { return control & (0x100 << bg_id); }
    bool ObjEnabled() const { return control & 0x1000; }
    bool WinEnabled(int win_id) const { return control & (0x2000 << win_id); }
    bool ObjWinEnabled() const { return control & 0x8000; }

    // Status flags
    static constexpr u16 vblank_flag = 0x01;
    static constexpr u16 hblank_flag = 0x02;
    static constexpr u16 vcount_flag = 0x04;
    static constexpr u16 vblank_irq  = 0x08;
    static constexpr u16 hblank_irq  = 0x10;
    static constexpr u16 vcount_irq  = 0x20;

    int VTrigger() const { return status >> 8; }

    // Blending flags
    enum class Effect {None = 0,
                       AlphaBlend = 1,
                       Brighten = 2,
                       Darken = 3};
    int FirstTargets() const { return blend_control & 0x3F; }
    Effect BlendMode() const { return static_cast<Effect>((blend_control >> 6) & 0x3); }
    int SecondTargets() const { return (blend_control >> 8) & 0x3F; }

    double FirstAlpha() const { return std::min(static_cast<double>(blend_alpha & 0x1F) / 16.0, 1.0); }
    double SecondAlpha() const { return std::min(static_cast<double>((blend_alpha >> 8) & 0x1F) / 16.0, 1.0); }

    double Intensity() const { return std::min(static_cast<double>(blend_fade & 0x1F) / 16.0, 1.0); }
};

} // End namespace Gba
