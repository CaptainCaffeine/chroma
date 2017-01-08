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

#pragma once

#include <string>
#include <fstream>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

namespace Core {

class Memory;
class CPU;
class Timer;
class LCD;

} // End namespace Core

namespace Log {

class Logging {
public:
    LogLevel log_level;

    Logging(LogLevel log_lvl, std::ofstream log_stream);

    void LogCPURegisterState(const Core::Memory& mem, const Core::CPU& cpu);
    void LogInterrupt(const Core::Memory& mem);
    void LogTimerRegisterState(const Core::Timer& timer);
    void LogLCDRegisterState(const Core::LCD& lcd);

    std::string Disassemble(const Core::Memory& mem, const u16 pc) const;

    void SwitchLogLevel();
private:
    LogLevel alt_level = LogLevel::None;

    std::ofstream log_file;
};

} // End namespace Log
