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

#include <iomanip>

#include "core/Logging.h"
#include "core/memory/Memory.h"
#include "core/Timer.h"
#include "core/LCD.h"
#include "core/cpu/CPU.h"

namespace Log {

Logging::Logging(const LogLevel log_lvl, std::ofstream log_stream)
    : log_level(log_lvl)
    , log_file(std::move(log_stream)) {}

void Logging::LogCPURegisterState(const Core::Memory& mem, const Core::CPU& cpu) {
    log_file << std::hex << std::uppercase;
    log_file << "\n";
    log_file << Disassemble(mem, cpu.pc) << "\n";
    log_file << "PC = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.pc);
    log_file << ", SP = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.sp);
    log_file << ", AF = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.Read16(Core::CPU::Reg16::AF));
    log_file << ", BC = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.Read16(Core::CPU::Reg16::BC));
    log_file << ", DE = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.Read16(Core::CPU::Reg16::DE));
    log_file << ", HL = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.Read16(Core::CPU::Reg16::HL));
    log_file << ", IF = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(0xFF0F));
    log_file << ", IE = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(0xFFFF));
    log_file << "\n";
}

void Logging::LogInterrupt(const Core::Memory& mem) {
    log_file << "\n";
    if (mem.IsPending(Interrupt::VBLANK)) {
        log_file << "VBLANK";
    } else if (mem.IsPending(Interrupt::STAT)) {
        log_file << "STAT";
    } else if (mem.IsPending(Interrupt::Timer)) {
        log_file << "Timer";
    } else if (mem.IsPending(Interrupt::Serial)) {
        log_file << "Serial";
    } else if (mem.IsPending(Interrupt::Joypad)) {
        log_file << "Joypad";
    }
    log_file << " Interrupt\n";
}

void Logging::LogTimerRegisterState(const Core::Timer& timer) {
    log_file << std::hex << std::uppercase;
    log_file << "DIV = 0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(timer.divider);
    log_file << ", TIMA = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(timer.tima);
    log_file << ", TMA = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(timer.tma);
    log_file << ", TAC = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(timer.tac);
    log_file << ", pt_inc = " << std::setw(1) << timer.prev_tima_inc;
    log_file << ", pt_val = " << std::setw(2) << static_cast<unsigned int>(timer.prev_tima_val);
    log_file << ", t_of = " << std::setw(1) << timer.tima_overflow;
    log_file << ", t_of_ni = " << std::setw(1) << timer.tima_overflow_not_interrupted << "\n";
}

void Logging::LogLCDRegisterState(const Core::LCD& lcd) {
    log_file << std::hex << std::uppercase;
    log_file << "LCDC = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(lcd.lcdc);
    log_file << ", STAT = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(lcd.stat);
    log_file << ", LY = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(lcd.ly);
    log_file << ", LYC = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(lcd.ly_compare);
    log_file << ", LCD On = 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(lcd.lcd_on);
    log_file << ", cycles = " << std::dec << std::setw(3) << lcd.scanline_cycles;
    log_file << ", bg_en = " << std::setw(1) << lcd.BGEnabled();
    log_file << ", win_en = " << std::setw(1) << lcd.WindowEnabled();
    log_file << ", stat_sig = " << std::setw(1) << lcd.stat_interrupt_signal << "\n";
}

} // End namespace Log
