// This file is a part of Chroma.
// Copyright (C) 2016-2018 Matthew Murray
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

#include "gb/logging/Logging.h"
#include "gb/core/GameBoy.h"
#include "gb/memory/Memory.h"
#include "gb/cpu/Cpu.h"

namespace Gb {

Logging::Logging(LogLevel level, const GameBoy& _gameboy)
        : gameboy(_gameboy)
        , alt_level(level) {
    // Leave log_stream unopened if logging disabled.
    if (level != LogLevel::None) {
        log_stream = std::ofstream("log.txt");

        if (!log_stream) {
            throw std::runtime_error("Error when attempting to open ./log.txt for writing.");
        }
    }
}

void Logging::LogInstruction(const Registers& regs, const u16 pc) {
    if (log_level == LogLevel::None) {
        return;
    }

    Disassemble(pc);

    if (log_level == LogLevel::Registers) {
        fmt::print(log_stream,  "A=0x{0:0>2X}", regs.reg8[1]);
        fmt::print(log_stream, " B=0x{0:0>2X}", regs.reg8[3]);
        fmt::print(log_stream, " C=0x{0:0>2X}", regs.reg8[2]);
        fmt::print(log_stream, " D=0x{0:0>2X}", regs.reg8[5]);
        fmt::print(log_stream, " E=0x{0:0>2X}", regs.reg8[4]);
        fmt::print(log_stream, " H=0x{0:0>2X}", regs.reg8[7]);
        fmt::print(log_stream, " L=0x{0:0>2X}", regs.reg8[6]);
        fmt::print(log_stream, " SP=0x{0:0>4X}", regs.reg16[4]);
        fmt::print(log_stream, " IF=0x{0:0>2X}", gameboy.mem->ReadMem(0xFF0F));
        fmt::print(log_stream, " IE=0x{0:0>2X} ", gameboy.mem->ReadMem(0xFFFF));
        fmt::print(log_stream, "{}", (regs.reg8[0] & 0x80) ? "Z" : "");
        fmt::print(log_stream, "{}", (regs.reg8[0] & 0x40) ? "N" : "");
        fmt::print(log_stream, "{}", (regs.reg8[0] & 0x20) ? "H" : "");
        fmt::print(log_stream, "{}", (regs.reg8[0] & 0x10) ? "C" : "");
        fmt::print(log_stream, "\n\n");
    }
}

void Logging::LogInterrupt() {
    if (log_level == LogLevel::None) {
        return;
    }

    auto InterruptString = [this]() {
        if (gameboy.mem->IsPending(Interrupt::VBlank)) {
            return "VBlank";
        } else if (gameboy.mem->IsPending(Interrupt::Stat)) {
            return "STAT";
        } else if (gameboy.mem->IsPending(Interrupt::Timer)) {
            return "Timer";
        } else if (gameboy.mem->IsPending(Interrupt::Serial)) {
            return "Serial";
        } else if (gameboy.mem->IsPending(Interrupt::Joypad)) {
            return "Joypad";
        }

        return "";
    };

    fmt::print(log_stream, "{} Interrupt\n", InterruptString());
}

void Logging::LogHalt() {
    if (log_level != LogLevel::None) {
        fmt::print(log_stream, "Halted for {} cycles\n", halt_cycles);
    }

    halt_cycles = 0;
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
        default:
            return "";
        }
    };

    fmt::print(log_stream, "Log level changed to {}\n", LogLevelString());
    fmt::print("Log level changed to {}\n", LogLevelString());
}

} // End namespace Gb
