// This file is a part of Chroma.
// Copyright (C) 2016 Matthew Murray
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

#include <iostream>
#include <iomanip>

#include "core/Timer.h"
#include "core/CPU.h"
#include "core/Disassembler.h"

namespace Core {

void CPU::PrintRegisterState() {
    std::cout << std::hex << std::uppercase;
    std::cout << "\n";
    std::cout << Disassemble(mem, pc) << "\n";
    std::cout << "PC = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(pc);
    std::cout << ", SP = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(sp);
    std::cout << ", AF = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(Read16(Reg16::AF));
    std::cout << ", BC = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(Read16(Reg16::BC));
    std::cout << ", DE = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(Read16(Reg16::DE));
    std::cout << ", HL = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(Read16(Reg16::HL));
    std::cout << "\n";
}

void Timer::PrintRegisterState() {
    std::cout << std::hex << std::uppercase;
    std::cout << "DIV = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(mem.ReadDIV());
    std::cout << ", TIMA = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(0xFF05));
    std::cout << ", TMA = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(0xFF06));
    std::cout << ", TAC = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(0xFF07));
    std::cout << ", IF = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(0xFF0F));
    std::cout << ", pt_inc = " << std::setw(1) << prev_tima_inc;
    std::cout << ", pt_val = " << std::setw(2) << static_cast<unsigned int>(prev_tima_val);
    std::cout << ", t_overflow = " << std::setw(1) << tima_overflow << "\n";
}

} // End namespace Core
