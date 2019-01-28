// This file is a part of Chroma.
// Copyright (C) 2018-2019 Matthew Murray
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
#include "gba/memory/MemDefs.h"

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

    enum class Timing {Immediate = 0,
                       VBlank    = 1,
                       HBlank    = 2,
                       Special   = 3};

    int Run();

    void WriteControl(const u16 data, const u16 mask);
    bool Active() const { return DmaEnabled() && !paused; }
    void Trigger(Timing event);
    bool WritingToFifo(int f) const { return dest == FIFO_A_L + 4 * f; }

private:
    const int id;
    Core& core;

    u32 source = 0x0;
    u32 dest = 0x0;
    int remaining_chunks = 1;

    bool bad_source = false;
    bool paused = false;
    bool starting = false;

    void ReloadWordCount();
    template<typename T>
    int Transfer(bool sequential);

    void DisableDma() { control &= ~0x8000; }
    bool FifoTimingEnabled() const { return StartTiming() == Timing::Special && (id == 1 || id == 2); }

    // Control flags
    enum AddrControl {Increment = 0,
                      Decrement = 1,
                      Fixed     = 2,
                      Reload    = 3};
    int DestControl() const { return (control >> 5) & 0x3; }
    int SourceControl() const { return (control >> 7) & 0x3; }
    bool RepeatEnabled() const { return control & 0x0200; }
    int TransferWidth() const { return ((control & 0x0400) || FifoTimingEnabled()) ? 4 : 2; }
    bool DrqEnabled() const { return control & 0x0800; }
    Timing StartTiming() const { return static_cast<Timing>((control >> 12) & 0x3); }
    bool InterruptEnabled() const { return control & 0x4000; }
    bool DmaEnabled() const { return control & 0x8000; }
};

} // End namespace Gba
