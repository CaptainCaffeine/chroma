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

    void Disassemble(const u16 pc);

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

    void LoadString(const std::string& into, const std::string& from);
    void LoadIncString(const std::string& into, const std::string& from);
    void LoadDecString(const std::string& into, const std::string& from);
    void LoadHighString(const std::string& into, const std::string& from);
    void PushString(const std::string& reg);
    void PopString(const std::string& reg);
    void AddString(const std::string& from);
    void AddString(const std::string& into, const std::string& from);
    void AdcString(const std::string& from);
    void SubString(const std::string& from);
    void SbcString(const std::string& from);
    void AndString(const std::string& with);
    void OrString(const std::string& with);
    void XorString(const std::string& with);
    void CompareString(const std::string& with);
    void IncString(const std::string& reg);
    void DecString(const std::string& reg);
    void JumpString(const std::string& addr);
    void JumpString(const std::string& cond, const std::string& addr);
    void RelJumpString(const std::string& addr);
    void RelJumpString(const std::string& cond, const std::string& addr);
    void CallString(const std::string& addr);
    void CallString(const std::string& cond, const std::string& addr);
    void ReturnInterruptString(const std::string& reti);
    void ReturnCondString(const std::string& cond);
    void RestartString(const std::string& addr);
    void RotLeftString(const std::string& carry, const std::string& reg);
    void RotRightString(const std::string& carry, const std::string& reg);
    void ShiftLeftString(const std::string& reg);
    void ShiftRightString(const std::string& a_or_l, const std::string& reg);
    void SwapString(const std::string& reg);
    void TestBitString(const std::string& bit, const std::string& reg);
    void ResetBitString(const std::string& bit, const std::string& reg);
    void SetBitString(const std::string& bit, const std::string& reg);
    void UnknownOpcodeString(const u8 opcode);
};

} // End namespace Gb
