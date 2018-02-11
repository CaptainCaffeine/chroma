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

#include "common/CommonTypes.h"
#include "gba/memory/IOReg.h"

namespace Gba {

class Core;

class Dma {
public:
    Dma(int _id, Core& _core);

    IOReg source_l   = {0x0000, 0x0000, 0xFFFE};
    IOReg source_h   = {0x0000, 0x0000, 0x0FFF};
    IOReg dest_l     = {0x0000, 0x0000, 0xFFFE};
    IOReg dest_h     = {0x0000, 0x0000, 0x07FF};
    IOReg word_count = {0x0000, 0x0000, 0x3FFF};
    IOReg control    = {0x0000, 0xF7E0, 0xF7E0};

    enum DmaTiming {Immediate = 0,
                    VBlank    = 1,
                    HBlank    = 2,
                    Special   = 3};

    int Run();

    void WriteControl(const u16 data, const u16 mask);
    bool Active() const { return (control & enable) && !paused; }
    void Trigger(DmaTiming event);

private:
    const int id;
    Core& core;

    u32 source = 0x0;
    u32 dest = 0x0;
    int remaining_chunks = 1;

    bool paused = false;
    bool starting = false;

    void ReloadWordCount();
    template<typename T>
    int Transfer(bool sequential);

    // Control flags
    static constexpr u16 repeat     = 0x0200;
    static constexpr u16 drq_flag   = 0x0800;
    static constexpr u16 irq_enable = 0x4000;
    static constexpr u16 enable     = 0x8000;

    enum AddrControl {Increment = 0,
                      Decrement = 1,
                      Fixed     = 2,
                      Reload    = 3};

    int DestControl() const { return (control >> 5) & 0x3; }
    int SourceControl() const { return (control >> 7) & 0x3; }
    int TransferWidth() const { return (control & 0x0400) ? 4 : 2; }
    int StartTiming() const { return (control >> 12) & 0x3; }
};

} // End namespace Gba
