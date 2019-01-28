// This file is a part of Chroma.
// Copyright (C) 2017-2019 Matthew Murray
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

#include "common/CommonTypes.h"

namespace Gba {

static constexpr int kbyte = 1024;
static constexpr int mbyte = kbyte * kbyte;

namespace Interrupt {
enum Interrupt {VBlank  = 0x0001,
                HBlank  = 0x0002,
                VCount  = 0x0004,
                Timer0  = 0x0008,
                Timer1  = 0x0010,
                Timer2  = 0x0020,
                Timer3  = 0x0040,
                Serial  = 0x0080,
                Dma0    = 0x0100,
                Dma1    = 0x0200,
                Dma2    = 0x0400,
                Dma3    = 0x0800,
                Keypad  = 0x1000,
                Gamepak = 0x2000};
}

enum class AccessType {Normal,
                       Opcode,
                       Dma};

namespace BaseAddr {
enum BaseAddr {Bios   = 0x0000'0000,
               XRam   = 0x0200'0000,
               IRam   = 0x0300'0000,
               IO     = 0x0400'0000,
               PRam   = 0x0500'0000,
               VRam   = 0x0600'0000,
               Oam    = 0x0700'0000,
               Rom    = 0x0800'0000,
               Eeprom = 0x0D00'0000,
               SRam   = 0x0E00'0000,
               Max    = 0x1000'0000};
}

enum IOAddr : u32 {DISPCNT     = 0x0400'0000,
                   GREENSWAP   = 0x0400'0002,
                   DISPSTAT    = 0x0400'0004,
                   VCOUNT      = 0x0400'0006,

                   BG0CNT      = 0x0400'0008,
                   BG1CNT      = 0x0400'000A,
                   BG2CNT      = 0x0400'000C,
                   BG3CNT      = 0x0400'000E,
                   BG0HOFS     = 0x0400'0010,
                   BG0VOFS     = 0x0400'0012,
                   BG1HOFS     = 0x0400'0014,
                   BG1VOFS     = 0x0400'0016,
                   BG2HOFS     = 0x0400'0018,
                   BG2VOFS     = 0x0400'001A,
                   BG3HOFS     = 0x0400'001C,
                   BG3VOFS     = 0x0400'001E,

                   BG2PA       = 0x0400'0020,
                   BG2PB       = 0x0400'0022,
                   BG2PC       = 0x0400'0024,
                   BG2PD       = 0x0400'0026,
                   BG2X_L      = 0x0400'0028,
                   BG2X_H      = 0x0400'002A,
                   BG2Y_L      = 0x0400'002C,
                   BG2Y_H      = 0x0400'002E,

                   BG3PA       = 0x0400'0030,
                   BG3PB       = 0x0400'0032,
                   BG3PC       = 0x0400'0034,
                   BG3PD       = 0x0400'0036,
                   BG3X_L      = 0x0400'0038,
                   BG3X_H      = 0x0400'003A,
                   BG3Y_L      = 0x0400'003C,
                   BG3Y_H      = 0x0400'003E,

                   WIN0H       = 0x0400'0040,
                   WIN1H       = 0x0400'0042,
                   WIN0V       = 0x0400'0044,
                   WIN1V       = 0x0400'0046,
                   WININ       = 0x0400'0048,
                   WINOUT      = 0x0400'004A,

                   MOSAIC      = 0x0400'004C,
                   BLDCNT      = 0x0400'0050,
                   BLDALPHA    = 0x0400'0052,
                   BLDY        = 0x0400'0054,

                   SOUND1CNT_L = 0x0400'0060,
                   SOUND1CNT_H = 0x0400'0062,
                   SOUND1CNT_X = 0x0400'0064,
                   INVALID_66  = 0x0400'0066,
                   SOUND2CNT_L = 0x0400'0068,
                   SOUND2CNT_H = 0x0400'006C,
                   INVALID_6E  = 0x0400'006E,
                   SOUND3CNT_L = 0x0400'0070,
                   SOUND3CNT_H = 0x0400'0072,
                   SOUND3CNT_X = 0x0400'0074,
                   INVALID_76  = 0x0400'0076,
                   SOUND4CNT_L = 0x0400'0078,
                   INVALID_7A  = 0x0400'007A,
                   SOUND4CNT_H = 0x0400'007C,
                   INVALID_7E  = 0x0400'007E,

                   SOUNDCNT_L  = 0x0400'0080,
                   SOUNDCNT_H  = 0x0400'0082,
                   SOUNDCNT_X  = 0x0400'0084,
                   INVALID_86  = 0x0400'0086,
                   SOUNDBIAS   = 0x0400'0088,
                   INVALID_8A  = 0x0400'008A,

                   WAVE_RAM0_L = 0x0400'0090,
                   WAVE_RAM0_H = 0x0400'0092,
                   WAVE_RAM1_L = 0x0400'0094,
                   WAVE_RAM1_H = 0x0400'0096,
                   WAVE_RAM2_L = 0x0400'0098,
                   WAVE_RAM2_H = 0x0400'009A,
                   WAVE_RAM3_L = 0x0400'009C,
                   WAVE_RAM3_H = 0x0400'009E,

                   FIFO_A_L    = 0x0400'00A0,
                   FIFO_A_H    = 0x0400'00A2,
                   FIFO_B_L    = 0x0400'00A4,
                   FIFO_B_H    = 0x0400'00A6,

                   DMA0SAD_L   = 0x0400'00B0,
                   DMA0SAD_H   = 0x0400'00B2,
                   DMA0DAD_L   = 0x0400'00B4,
                   DMA0DAD_H   = 0x0400'00B6,
                   DMA0CNT_L   = 0x0400'00B8,
                   DMA0CNT_H   = 0x0400'00BA,

                   DMA1SAD_L   = 0x0400'00BC,
                   DMA1SAD_H   = 0x0400'00BE,
                   DMA1DAD_L   = 0x0400'00C0,
                   DMA1DAD_H   = 0x0400'00C2,
                   DMA1CNT_L   = 0x0400'00C4,
                   DMA1CNT_H   = 0x0400'00C6,

                   DMA2SAD_L   = 0x0400'00C8,
                   DMA2SAD_H   = 0x0400'00CA,
                   DMA2DAD_L   = 0x0400'00CC,
                   DMA2DAD_H   = 0x0400'00CE,
                   DMA2CNT_L   = 0x0400'00D0,
                   DMA2CNT_H   = 0x0400'00D2,

                   DMA3SAD_L   = 0x0400'00D4,
                   DMA3SAD_H   = 0x0400'00D6,
                   DMA3DAD_L   = 0x0400'00D8,
                   DMA3DAD_H   = 0x0400'00DA,
                   DMA3CNT_L   = 0x0400'00DC,
                   DMA3CNT_H   = 0x0400'00DE,

                   TM0CNT_L    = 0x0400'0100,
                   TM0CNT_H    = 0x0400'0102,
                   TM1CNT_L    = 0x0400'0104,
                   TM1CNT_H    = 0x0400'0106,
                   TM2CNT_L    = 0x0400'0108,
                   TM2CNT_H    = 0x0400'010A,
                   TM3CNT_L    = 0x0400'010C,
                   TM3CNT_H    = 0x0400'010E,

                   SIOMULTI0   = 0x0400'0120, // Also SIODATA32_L
                   SIOMULTI1   = 0x0400'0122, // Also SIODATA32_H
                   SIOMULTI2   = 0x0400'0124,
                   SIOMULTI3   = 0x0400'0126,
                   SIOCNT      = 0x0400'0128,
                   SIOMLTSEND  = 0x0400'012A, // Also SIODATA8

                   KEYINPUT    = 0x0400'0130,
                   KEYCNT      = 0x0400'0132,

                   RCNT        = 0x0400'0134,
                   JOYCNT      = 0x0400'0140,
                   JOYRECV_L   = 0x0400'0150,
                   JOYRECV_H   = 0x0400'0152,
                   JOYTRANS_L  = 0x0400'0154,
                   JOYTRANS_H  = 0x0400'0156,
                   JOYSTAT     = 0x0400'0158,

                   IE          = 0x0400'0200,
                   IF          = 0x0400'0202,
                   WAITCNT     = 0x0400'0204,
                   IME         = 0x0400'0208,
                   HALTCNT     = 0x0400'0300}; // Also POSTFLG

} // End namespace Gba
