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

#include <array>

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"

// Forward declaration for cross-namespace friend declaration.
namespace Log { class Logging; }

namespace Core {

class Memory;
class GameBoy;

class CPU {
    friend class Log::Logging;
public:
    CPU(Memory& memory);

    int RunFor(int cycles);

    void LinkToGameBoy(GameBoy* gb) { gameboy = gb; }
    void EnableInterruptsDelayed();

    bool IsHalted() const { return cpu_mode == CPUMode::Halted; }
private:
    Memory& mem;
    GameBoy* gameboy;

    // Registers
    union Registers {
        u8 reg8[10];
        u16 reg16[5];
    };

    u16 pc = 0x0100;
    Registers regs;
    using Reg8Addr = std::size_t;
    using Reg16Addr = std::size_t;
    static constexpr Reg8Addr A = 1, F = 0, B = 3, C = 2, D = 5, E = 4, H = 7, L = 6;
    static constexpr Reg16Addr AF = 0, BC = 1, DE = 2, HL = 3, SP = 4;

    static Reg8Addr ToReg8AddrLo(Reg16Addr r) { return r * 2; }
    static Reg8Addr ToReg8AddrHi(Reg16Addr r) { return r * 2 + 1; }

    // Interpreter execution
    enum class CPUMode {Running, Halted, HaltBug, Stopped};
    CPUMode cpu_mode = CPUMode::Running;
    unsigned int speed_switch_cycles = 0;
    unsigned int ExecuteNext(const u8 opcode);
    void StoppedTick();

    // Interrupts
    bool interrupt_master_enable = true;
    bool enable_interrupts_delayed = false;

    int HandleInterrupts();
    void ServiceInterrupt(const u16 addr);

    // Memory access
    u8 ReadMemAndTick(const u16 addr);
    void WriteMemAndTick(const u16 addr, const u8 val);

    u8 GetImmediateByte();
    s8 GetImmediateSignedByte();
    u16 GetImmediateWord();

    // Ops
    // 8-bit loads
    void Load8(Reg8Addr r, u8 val);
    void Load8FromMem(Reg8Addr r, u16 addr);
    void Load8IntoMem(u16 addr, u8 val);

    // 16-bit loads
    void Load16(Reg16Addr r, u16 val);
    void LoadHLIntoSP();
    void LoadSPnIntoHL(s8 val);
    void LoadSPIntoMem(u16 addr);
    void Push(Reg16Addr r);
    void Pop(Reg16Addr r);

    // 8-bit arithmetic
    void AddImmediate(u8 val);
    void Add(Reg8Addr r);
    void AddFromMemAtHL();

    void AddImmediateWithCarry(u8 val);
    void AddWithCarry(Reg8Addr r);
    void AddFromMemAtHLWithCarry();

    void SubImmediate(u8 val);
    void Sub(Reg8Addr r);
    void SubFromMemAtHL();

    void SubImmediateWithCarry(u8 val);
    void SubWithCarry(Reg8Addr r);
    void SubFromMemAtHLWithCarry();

    void IncReg8(Reg8Addr r);
    void IncMemAtHL();
    void DecReg8(Reg8Addr r);
    void DecMemAtHL();

    // 8-bit logic
    void AndImmediate(u8 val);
    void And(Reg8Addr r);
    void AndFromMemAtHL();

    void OrImmediate(u8 val);
    void Or(Reg8Addr r);
    void OrFromMemAtHL();

    void XorImmediate(u8 val);
    void Xor(Reg8Addr r);
    void XorFromMemAtHL();

    void CompareImmediate(u8 val);
    void Compare(Reg8Addr r);
    void CompareFromMemAtHL();

    // 16-bit arithmetic
    void AddHL(Reg16Addr r);
    void AddSP(s8 val);
    void IncReg16(Reg16Addr r);
    void DecReg16(Reg16Addr r);

    // Miscellaneous arithmetic
    void DecimalAdjustA();
    void ComplementA();
    void SetCarry();
    void ComplementCarry();

    // Rotates and Shifts
    void RotateLeft(Reg8Addr r);
    void RotateLeftMemAtHL();
    void RotateLeftThroughCarry(Reg8Addr r);
    void RotateLeftMemAtHLThroughCarry();
    void RotateRight(Reg8Addr r);
    void RotateRightMemAtHL();
    void RotateRightThroughCarry(Reg8Addr r);
    void RotateRightMemAtHLThroughCarry();
    void ShiftLeft(Reg8Addr r);
    void ShiftLeftMemAtHL();
    void ShiftRightArithmetic(Reg8Addr r);
    void ShiftRightArithmeticMemAtHL();
    void ShiftRightLogical(Reg8Addr r);
    void ShiftRightLogicalMemAtHL();
    void SwapNybbles(Reg8Addr r);
    void SwapMemAtHL();

    // Bit manipulation
    void TestBit(unsigned int bit, Reg8Addr r);
    void TestBitOfMemAtHL(unsigned int bit);
    void ResetBit(unsigned int bit, Reg8Addr r);
    void ResetBitOfMemAtHL(unsigned int bit);
    void SetBit(unsigned int bit, Reg8Addr r);
    void SetBitOfMemAtHL(unsigned int bit);

    // Jumps
    void Jump(u16 addr);
    void JumpToHL();
    void RelativeJump(s8 val);

    // Calls and Returns
    void Call(u16 addr);
    void Return();

    // System control
    void Halt();
    void Stop();

    // Flags
    static constexpr u8 zero = 0x80, sub = 0x40, half = 0x20, carry = 0x10;

    void SetZero(bool val)  { (val) ? (regs.reg8[F] |= zero)  : (regs.reg8[F] &= ~zero);  }
    void SetSub(bool val)   { (val) ? (regs.reg8[F] |= sub)   : (regs.reg8[F] &= ~sub);   }
    void SetHalf(bool val)  { (val) ? (regs.reg8[F] |= half)  : (regs.reg8[F] &= ~half);  }
    void SetCarry(bool val) { (val) ? (regs.reg8[F] |= carry) : (regs.reg8[F] &= ~carry); }

    // Getters
    u8 Zero()  const { return (regs.reg8[F] & zero)  >> 7; }
    u8 Sub()   const { return (regs.reg8[F] & sub)   >> 6; }
    u8 Half()  const { return (regs.reg8[F] & half)  >> 5; }
    u8 Carry() const { return (regs.reg8[F] & carry) >> 4; }
};

} // End namespace Core
