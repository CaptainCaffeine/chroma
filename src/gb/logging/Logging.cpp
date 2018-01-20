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
#include <stdexcept>
#include <fmt/format.h>

#include "gb/logging/Logging.h"
#include "gb/memory/Memory.h"
#include "gb/hardware/Timer.h"
#include "gb/lcd/LCD.h"
#include "gb/cpu/CPU.h"

namespace Gb {

Logging::Logging(LogLevel log_lvl) : log_level(log_lvl) {
    // Leave log_stream unopened if logging disabled.
    if (log_level != LogLevel::None) {
        log_stream = std::ofstream("log.txt");

        if (!log_stream) {
            throw std::runtime_error("Error when attempting to open ./log.txt for writing.");
        }
    }
}

void Logging::LogCPURegisterState(const Memory& mem, const CPU& cpu) {
    fmt::MemoryWriter cpu_log;

    if (cpu.IsHalted()) {
        cpu_log.write("\nHalted\n");
    } else {
        Disassemble(cpu_log, mem, cpu.pc);
    }

    cpu_log.write( "PC=0x{0:0>4X}", cpu.pc);
    cpu_log.write(" SP=0x{0:0>4X}", cpu.regs.reg16[CPU::SP]);
    cpu_log.write(" AF=0x{0:0>4X}", cpu.regs.reg16[CPU::AF]);
    cpu_log.write(" BC=0x{0:0>4X}", cpu.regs.reg16[CPU::BC]);
    cpu_log.write(" DE=0x{0:0>4X}", cpu.regs.reg16[CPU::DE]);
    cpu_log.write(" HL=0x{0:0>4X}", cpu.regs.reg16[CPU::HL]);
    cpu_log.write(" IF=0x{0:0>4X}", mem.ReadMem(0xFF0F));
    cpu_log.write(" IE=0x{0:0>4X}", mem.ReadMem(0xFFFF));
    cpu_log.write("\n");

    log_stream << cpu_log.c_str();
}

void Logging::LogInterrupt(const Memory& mem) {
    auto InterruptString = [&mem]() {
        if (mem.IsPending(Interrupt::VBLANK)) {
            return "VBLANK";
        } else if (mem.IsPending(Interrupt::STAT)) {
            return "STAT";
        } else if (mem.IsPending(Interrupt::Timer)) {
            return "Timer";
        } else if (mem.IsPending(Interrupt::Serial)) {
            return "Serial";
        } else if (mem.IsPending(Interrupt::Joypad)) {
            return "Joypad";
        }

        return "";
    };

    log_stream << fmt::format("\n{} Interrupt\n", InterruptString());
}

void Logging::LogTimerRegisterState(const Timer& timer) {
    fmt::MemoryWriter timer_log;

    timer_log.write("DIV=0x{0:0>4X}", timer.divider);
    timer_log.write(" TIMA=0x{0:0>2X}", timer.tima);
    timer_log.write(" TMA=0x{0:0>2X}", timer.tma);
    timer_log.write(" TAC=0x{0:0>2X}", timer.tac);
    timer_log.write(" p_inc={0:X}", timer.prev_tima_inc);
    timer_log.write(" p_val={0:0>2X}", timer.prev_tima_val);
    timer_log.write(" of={0:X}", timer.tima_overflow);
    timer_log.write(" of_ni={0:X}\n", timer.tima_overflow_not_interrupted);

    log_stream << timer_log.c_str();
}

void Logging::LogLCDRegisterState(const LCD& lcd) {
    fmt::MemoryWriter lcd_log;

    lcd_log.write("LCDC=0x{0:0>2X}", lcd.lcdc);
    lcd_log.write(" STAT=0x{0:0>2X}", lcd.stat);
    lcd_log.write(" LY=0x{0:0>2X}", lcd.ly);
    lcd_log.write(" LYC=0x{0:0>2X}", lcd.ly_compare);
    lcd_log.write(" cycles={0:0>3d}", lcd.scanline_cycles);
    lcd_log.write(" stat_sig={0:X}\n", lcd.stat_interrupt_signal);

    log_stream << lcd_log.c_str();
}

void Logging::SwitchLogLevel() {
    // Don't spam if logging not enabled.
    if (log_level == alt_level) {
        return;
    }

    std::swap(log_level, alt_level);

    auto LogLevelString = [log_level = this->log_level]() {
        switch (log_level) {
        case LogLevel::None:
            return "None";
        case LogLevel::Trace:
            return "Trace";
        case LogLevel::Registers:
            return "Registers";
        case LogLevel::LCD:
            return "LCD";
        case LogLevel::Timer:
            return "Timer";
        default:
            return "";
        }
    };

    const std::string switch_str{fmt::format("\nLog level changed to {}\n", LogLevelString())};

    log_stream << switch_str;
    fmt::print(switch_str);
}

} // End namespace Gb
