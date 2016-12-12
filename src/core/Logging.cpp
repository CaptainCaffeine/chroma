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
#include "core/LCD.h"
#include "core/cpu/CPU.h"
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
    std::cout << ", IF = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(0xFF0F));
    std::cout << ", IE = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(0xFFFF));
    std::cout << "\n";
}

void CPU::PrintInterrupt() {
    std::cout << "\n";
    if (mem.IsPending(Interrupt::VBLANK)) {
        std::cout << "VBLANK";
    } else if (mem.IsPending(Interrupt::STAT)) {
        std::cout << "STAT";
    } else if (mem.IsPending(Interrupt::Timer)) {
        std::cout << "Timer";
    } else if (mem.IsPending(Interrupt::Serial)) {
        std::cout << "Serial";
    } else if (mem.IsPending(Interrupt::Joypad)) {
        std::cout << "Joypad";
    }
    std::cout << " Interrupt\n";
}

void Timer::PrintRegisterState() {
    std::cout << std::hex << std::uppercase;
    std::cout << "DIV = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(divider);
    std::cout << ", TIMA = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(tima);
    std::cout << ", TMA = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(tma);
    std::cout << ", TAC = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(tac);
    std::cout << ", pt_inc = " << std::setw(1) << prev_tima_inc;
    std::cout << ", pt_val = " << std::setw(2) << static_cast<unsigned int>(prev_tima_val);
    std::cout << ", t_of = " << std::setw(1) << tima_overflow;
    std::cout << ", t_of_ni = " << std::setw(1) << tima_overflow_not_interrupted << "\n";
}

void LCD::PrintRegisterState() {
    std::cout << std::hex << std::uppercase;
    std::cout << "LCDC = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(lcdc);
    std::cout << ", STAT = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(stat);
    std::cout << ", LY = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(ly);
    std::cout << ", LYC = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(ly_compare);
    std::cout << ", LCD On = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(lcd_on);
    std::cout << ", cycles = " << std::dec << std::setw(3) << scanline_cycles;
    std::cout << ", bg_en = " << std::setw(1) << BGEnabled();
    std::cout << ", win_en = " << std::setw(1) << WindowEnabled();
    std::cout << ", stat_sig = " << std::setw(1) << stat_interrupt_signal << "\n";
}

} // End namespace Core
