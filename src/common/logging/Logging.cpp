// This file is a part of Chroma.
// Copyright (C) 2016-2017 Matthew Murray
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

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <stdexcept>

#include "common/logging/Logging.h"
#include "core/memory/Memory.h"
#include "core/Timer.h"
#include "core/lcd/LCD.h"
#include "core/cpu/CPU.h"

namespace Log {

Logging::Logging(LogLevel log_lvl) : log_level(log_lvl) {
    // Leave log_stream unopened if logging disabled.
    if (log_level != LogLevel::None) {
        log_stream = std::ofstream("log.txt");

        if (!log_stream) {
            throw std::runtime_error("Error when attempting to open ./log.txt for writing.");
        }
    }
}

void Logging::LogCPURegisterState(const Core::Memory& mem, const Core::CPU& cpu) {
    log_stream << std::hex << std::uppercase;

    if (cpu.IsHalted()) {
        log_stream << "\nHalted\n";
    } else {
        log_stream << "\n" << Disassemble(mem, cpu.pc) << "\n";
    }

    log_stream << "PC=0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.pc);
    log_stream << " SP=0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.regs.reg16[cpu.SP]);
    log_stream << " AF=0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.regs.reg16[cpu.AF]);
    log_stream << " BC=0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.regs.reg16[cpu.BC]);
    log_stream << " DE=0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.regs.reg16[cpu.DE]);
    log_stream << " HL=0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(cpu.regs.reg16[cpu.HL]);
    log_stream << " IF=0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(0xFF0F));
    log_stream << " IE=0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(0xFFFF));
    log_stream << "\n";
}

void Logging::LogInterrupt(const Core::Memory& mem) {
    log_stream << "\n";
    if (mem.IsPending(Interrupt::VBLANK)) {
        log_stream << "VBLANK";
    } else if (mem.IsPending(Interrupt::STAT)) {
        log_stream << "STAT";
    } else if (mem.IsPending(Interrupt::Timer)) {
        log_stream << "Timer";
    } else if (mem.IsPending(Interrupt::Serial)) {
        log_stream << "Serial";
    } else if (mem.IsPending(Interrupt::Joypad)) {
        log_stream << "Joypad";
    }
    log_stream << " Interrupt\n";
}

void Logging::LogTimerRegisterState(const Core::Timer& timer) {
    log_stream << std::hex << std::uppercase;
    log_stream << "DIV=0x" << std::setfill('0') << std::setw(4) << static_cast<unsigned int>(timer.divider);
    log_stream << " TIMA=0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(timer.tima);
    log_stream << " TMA=0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(timer.tma);
    log_stream << " TAC=0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(timer.tac);
    log_stream << " p_inc=" << std::setw(1) << timer.prev_tima_inc;
    log_stream << " p_val=" << std::setw(2) << static_cast<unsigned int>(timer.prev_tima_val);
    log_stream << " of=" << std::setw(1) << timer.tima_overflow;
    log_stream << " of_ni=" << std::setw(1) << timer.tima_overflow_not_interrupted << "\n";
}

void Logging::LogLCDRegisterState(const Core::LCD& lcd) {
    log_stream << std::hex << std::uppercase;
    log_stream << "LCDC=0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(lcd.lcdc);
    log_stream << " STAT=0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(lcd.stat);
    log_stream << " LY=0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(lcd.ly);
    log_stream << " LYC=0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(lcd.ly_compare);
    log_stream << " cycles=" << std::dec << std::setw(3) << lcd.scanline_cycles;
    log_stream << " stat_sig=" << std::setw(1) << lcd.stat_interrupt_signal << "\n";
}

void Logging::SwitchLogLevel() {
    // Don't spam if logging not enabled.
    if (log_level == alt_level) {
        return;
    }

    std::swap(log_level, alt_level);

    std::string switch_str = "Log level changed to ";
    switch (log_level) {
    case LogLevel::None:
        switch_str += "None";
        break;
    case LogLevel::Regular:
        switch_str += "Regular";
        break;
    case LogLevel::LCD:
        switch_str += "LCD";
        break;
    case LogLevel::Timer:
        switch_str += "Timer";
        break;
    }
    switch_str += "\n";

    log_stream << "\n" << switch_str;
    std::cout << switch_str;
}

} // End namespace Log
