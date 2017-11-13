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

#include "gba/cpu/Cpu.h"
#include "gba/cpu/Decoder.h"
#include "gba/memory/Memory.h"

namespace Gba {

Cpu::Cpu(Memory& memory)
        : mem(memory)
        , decoder(std::make_unique<Decoder>()) { regs[pc] = 0x80000B8; }

// Needed to declare unique_ptrs with forward declarations in the header file.
Cpu::~Cpu() = default;

void Cpu::Execute(int cycles) {
    while (cycles > 0) {
        if (cpsr & thumb_mode) {
            Thumb opcode = mem.ReadMem<Thumb>(regs[pc]);
            regs[pc] += 2;

            decoder->DecodeThumb(opcode);
        } else {
            Arm opcode = mem.ReadMem<Arm>(regs[pc]);
            regs[pc] += 4;

            decoder->DecodeArm(opcode);
        }

        --cycles;
    }
}

} // End namespace Gba
