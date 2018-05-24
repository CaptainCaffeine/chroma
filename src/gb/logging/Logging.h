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

#pragma once

#include <string>
#include <fstream>
#include <utility>
#include <fmt/ostream.h>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

namespace Gb {

class GameBoy;
union Registers;

class Logging {
public:
    Logging(LogLevel level, const GameBoy& _gameboy);

    void LogInstruction(const Registers& regs, const u16 pc);
    void LogInterrupt();

    template<typename... Args>
    void Log(const std::string& log_msg, Args&&... args) {
        if (log_level != LogLevel::None) {
            fmt::print(log_stream, log_msg, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void LogAlways(const std::string& log_msg, Args&&... args) {
        fmt::print(log_stream, log_msg, std::forward<Args>(args)...);
    }

    void IncHaltCycles(int cycles) { halt_cycles += cycles; }
    void LogHalt();

    void Disassemble(fmt::MemoryWriter& instr_stream, const u16 pc) const;

    void SwitchLogLevel();

private:
    const GameBoy& gameboy;

    LogLevel log_level = LogLevel::None;
    LogLevel alt_level;

    int halt_cycles = 0;

    std::ofstream log_stream;

    std::string NextByteAsStr(const u16 pc) const;
    std::string NextSignedByteAsStr(const u16 pc) const;
    std::string NextWordAsStr(const u16 pc) const;
};

} // End namespace Gb
