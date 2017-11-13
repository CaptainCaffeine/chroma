// This file is a part of Chroma.
// Copyright (C) 2017 Matthew Murray
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

#include <array>
#include <memory>

#include "common/CommonTypes.h"

namespace Gba {

class Memory;
class Decoder;

class Cpu {
public:
    Cpu(Memory& memory);
    ~Cpu();

    void Execute(int cycles);
private:
    Memory& mem;
    std::unique_ptr<Decoder> decoder;

    std::array<u32, 16> regs{};
    u32 cpsr = irq_disable | fiq_disable | svc;

    // Constants
    static constexpr std::size_t sp = 13, lr = 14, pc = 15;

    enum CpsrMasks : u32 {sign        = 0x8000'0000,
                          zero        = 0x4000'0000,
                          carry       = 0x2000'0000,
                          overflow    = 0x1000'0000,
                          irq_disable = 0x0000'0080,
                          fiq_disable = 0x0000'0040,
                          thumb_mode  = 0x0000'0020,
                          mode        = 0x0000'001F};

    enum CpuMode : u32 {user   = 0x10,
                        fiq    = 0x11,
                        irq    = 0x12,
                        svc    = 0x13,
                        abort  = 0x17,
                        undef  = 0x1B,
                        system = 0x1F};
};

} // End namespace Gba
