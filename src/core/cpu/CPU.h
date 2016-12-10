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

#include "common/CommonTypes.h"
#include "common/CommonEnums.h"
#include "core/memory/Memory.h"
#include "core/Timer.h"
#include "core/LCD.h"
#include "core/cpu/Flags.h"

namespace Core {

class CPU {
public:
    CPU(Memory& memory, Timer& tima, LCD& display);

    void RunFor(int cycles);
private:
    enum class Reg8 {A, B, C, D, E, H, L};
    enum class Reg16 {AF, BC, DE, HL, SP};
    enum class CPUMode {Running, Halted, HaltBug};

    Memory& mem;
    Timer& timer;
    LCD& lcd;

    // Registers
    u8 a, b, c, d, e, h, l;
    u16 sp = 0xFFFE, pc = 0x0100;
    Flags f;

    // Internal CPU status
    bool interrupt_master_enable = true;
    bool enable_interrupts_delayed = false;
    CPUMode cpu_mode = CPUMode::Running;

    int HandleInterrupts();
    void ServiceInterrupt(const u16 addr);
    void HardwareTick(unsigned int cycles);
    unsigned int ExecuteNext(const u8 opcode);

    // Register interaction
    u8 Read8(Reg8 R) const; // Only used twice in ResetBit and SetBit
    u16 Read16(Reg16 R) const;
    void Write8(Reg8 R, u8 val);
    void Write16(Reg16 R, u16 val);

    u8 ReadMemAtHL();
    void WriteMemAtHL(const u8 val);

    // Fetching Immediate Values
    u8 GetImmediateByte();
    s8 GetImmediateSignedByte();
    u16 GetImmediateWord();

    // Debug
    void PrintRegisterState();
    void PrintInterrupt();
    void BlarggRamDebug();
    bool stop_printing = false;

    // Serial Stub
    void DisconnectedSerial();

    // Ops
    // 8-bit loads
    void Load8(Reg8 R, u8 immediate);
    void Load8FromMem(Reg8 R, u16 addr);
    void Load8FromMemAtHL(Reg8 R);
    void Load8IntoMem(Reg16 R, u8 immediate);
    void LoadAIntoMem(u16 addr);

    // 16-bit loads
    void Load16(Reg16 R, u16 immediate);
    void LoadHLIntoSP();
    void LoadSPnIntoHL(s8 immediate);
    void LoadSPIntoMem(u16 addr);
    void Push(Reg16 R);
    void Pop(Reg16 R);

    // 8-bit arithmetic
    void Add(u8 immediate);
    void AddFromMemAtHL();
    void AddWithCarry(u8 immediate);
    void AddFromMemAtHLWithCarry();
    void Sub(u8 immediate);
    void SubFromMemAtHL();
    void SubWithCarry(u8 immediate);
    void SubFromMemAtHLWithCarry();
    void IncReg(Reg8 R);
    void IncMemAtHL();
    void DecReg(Reg8 R);
    void DecMemAtHL();

    // 8-bit logic
    void And(u8 immediate);
    void AndFromMemAtHL();
    void Or(u8 immediate);
    void OrFromMemAtHL();
    void Xor(u8 immediate);
    void XorFromMemAtHL();
    void Compare(u8 immediate);
    void CompareFromMemAtHL();

    // 16-bit arithmetic
    void AddHL(Reg16 R);
    void AddSP(s8 immediate);
    void IncHL(); // No hardware tick
    void IncReg(Reg16 R);
    void DecHL(); // No hardware tick
    void DecReg(Reg16 R);

    // Miscellaneous arithmetic
    void DecimalAdjustA();
    void ComplementA();
    void SetCarry();
    void ComplementCarry();

    // Rotates and Shifts
    void RotateLeft(Reg8 R);
    void RotateLeftMemAtHL();
    void RotateLeftThroughCarry(Reg8 R);
    void RotateLeftMemAtHLThroughCarry();
    void RotateRight(Reg8 R);
    void RotateRightMemAtHL();
    void RotateRightThroughCarry(Reg8 R);
    void RotateRightMemAtHLThroughCarry();
    void ShiftLeft(Reg8 R);
    void ShiftLeftMemAtHL();
    void ShiftRightArithmetic(Reg8 R);
    void ShiftRightArithmeticMemAtHL();
    void ShiftRightLogical(Reg8 R);
    void ShiftRightLogicalMemAtHL();
    void SwapNybbles(Reg8 R);
    void SwapMemAtHL();

    // Bit manipulation
    void TestBit(unsigned int bit, u8 immediate);
    void TestBitOfMemAtHL(unsigned int bit);
    void ResetBit(unsigned int bit, Reg8 R);
    void ResetBitOfMemAtHL(unsigned int bit);
    void SetBit(unsigned int bit, Reg8 R);
    void SetBitOfMemAtHL(unsigned int bit);

    // Jumps
    void Jump(u16 addr);
    void JumpToHL();
    void RelativeJump(s8 immediate);

    // Calls and Returns
    void Call(u16 addr);
    void Return();

    // System control
    void Halt();
    void Stop();
};

} // End namespace Core
