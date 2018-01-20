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

#pragma once

#include <fstream>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

namespace fmt {

template<typename Char, typename Allocator>
class BasicMemoryWriter;

typedef BasicMemoryWriter<char, std::allocator<char>> MemoryWriter;

}

namespace Gb {

class Memory;
class CPU;
class Timer;
class LCD;

class Logging {
public:
    LogLevel log_level;

    Logging(LogLevel log_lvl);

    void LogCPURegisterState(const Memory& mem, const CPU& cpu);
    void LogInterrupt(const Memory& mem);
    void LogTimerRegisterState(const Timer& timer);
    void LogLCDRegisterState(const LCD& lcd);

    void Disassemble(fmt::MemoryWriter& instr_stream, const Memory& mem, const u16 pc) const;

    void SwitchLogLevel();
private:
    LogLevel alt_level = LogLevel::None;

    std::ofstream log_stream;
};

} // End namespace Gb
