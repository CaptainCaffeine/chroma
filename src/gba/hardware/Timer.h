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

class Timer {
public:
    Timer(int _id, Core& _core);

    IOReg counter = {0x0000, 0xFFFF, 0x0000};
    IOReg reload  = {0x0000, 0x0000, 0xFFFF};
    IOReg control = {0x0000, 0x00C7, 0x00C7};

    const int id;

    void Tick(int cycles);
    void CounterTick();
    void InactiveTick(int cycles) { timer_clock += cycles; }
    int NextEvent() const;

    u16 ReadCounter();
    void WriteReload(const u16 data, const u16 mask);
    void WriteControl(const u16 data, const u16 mask);

    bool CascadeEnabled() const { return control & 0x0004; }
    // We only check the timer counter if the timer is enabled and cascade is disabled.
    bool TimerNotRunning() const { return (control & 0x0084) != 0x0080; }
private:
    Core& core;

    u64 timer_clock = 0;
    int delay = 0;
    int cycles_per_tick = 0;

    bool TimerEnabled() const { return control & 0x0080; }
    bool InterruptEnabled() const { return control & 0x0040; }
};

} // End namespace Gba
