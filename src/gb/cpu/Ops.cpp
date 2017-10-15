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

#include <stdexcept>

#include "gb/cpu/CPU.h"
#include "gb/memory/Memory.h"
#include "gb/core/GameBoy.h"

namespace Gb {

// 8-bit Load operations
void CPU::Load8Immediate(Reg8Addr r, u8 val) {
    regs.reg8[r] = val;
}

void CPU::Load8(Reg8Addr r1, Reg8Addr r2) {
    regs.reg8[r1] = regs.reg8[r2];
}

void CPU::Load8FromMem(Reg8Addr r, u16 addr) {
    regs.reg8[r] = ReadMemAndTick(addr);
}

void CPU::Load8IntoMemImmediate(u16 addr, u8 val) {
    WriteMemAndTick(addr, val);
}

void CPU::Load8IntoMem(u16 addr, Reg8Addr r) {
    WriteMemAndTick(addr, regs.reg8[r]);
}

// 16-bit Load operations
void CPU::Load16Immediate(Reg16Addr r, u16 val) {
    regs.reg16[r] = val;
}

void CPU::LoadHLIntoSP() {
    regs.reg16[SP] = regs.reg16[HL];
    gameboy->HardwareTick(4);
}

void CPU::LoadSPnIntoHL(s8 val) {
    // The half carry & carry flags for this instruction are set by adding the value as an *unsigned* byte to the
    // lower byte of sp. The addition itself is done with the value as a signed byte.
    SetZero(false);
    SetSub(false);
    SetHalf(((regs.reg16[SP] & 0x000F) + (static_cast<u8>(val) & 0x0F)) & 0x0010);
    SetCarry(((regs.reg16[SP] & 0x00FF) + static_cast<u8>(val)) & 0x0100);

    // val will be sign-extended.
    regs.reg16[HL] = regs.reg16[SP] + val;
    
    // Internal delay
    gameboy->HardwareTick(4);
}

void CPU::LoadSPIntoMem(u16 addr) {
    WriteMemAndTick(addr, regs.reg8[ToReg8AddrLo(SP)]);
    WriteMemAndTick(addr + 1, regs.reg8[ToReg8AddrHi(SP)]);
}

void CPU::Push(Reg16Addr r) {
    // Internal delay
    gameboy->HardwareTick(4);

    WriteMemAndTick(--regs.reg16[SP], regs.reg8[ToReg8AddrHi(r)]);
    WriteMemAndTick(--regs.reg16[SP], regs.reg8[ToReg8AddrLo(r)]);
}

void CPU::Pop(Reg16Addr r) {
    regs.reg8[ToReg8AddrLo(r)] = ReadMemAndTick(regs.reg16[SP]++);
    regs.reg8[ToReg8AddrHi(r)] = ReadMemAndTick(regs.reg16[SP]++);

    if (r == AF) {
        // The low nybble of the flags register is always 0.
        regs.reg8[F] &= 0xF0;
    }
}

// 8-bit Add operations
void CPU::AddImmediate(u8 val) {
    u16 tmp16 = regs.reg8[A] + val;
    SetHalf(((regs.reg8[A] & 0x0F) + (val & 0x0F)) & 0x10);
    SetCarry(tmp16 & 0x0100);
    SetZero(!(tmp16 & 0x00FF));
    SetSub(false);

    regs.reg8[A] = static_cast<u8>(tmp16);
}

void CPU::Add(Reg8Addr r) {
    AddImmediate(regs.reg8[r]);
}

void CPU::AddFromMemAtHL() {
    AddImmediate(ReadMemAndTick(regs.reg16[HL]));
}

void CPU::AddImmediateWithCarry(u8 val) {
    u16 tmp16 = regs.reg8[A] + val + Carry();
    SetHalf(((regs.reg8[A] & 0x0F) + (val & 0x0F) + Carry()) & 0x10);
    SetCarry(tmp16 & 0x0100);
    SetZero(!(tmp16 & 0x00FF));
    SetSub(false);

    regs.reg8[A] = static_cast<u8>(tmp16);
}

void CPU::AddWithCarry(Reg8Addr r) {
    AddImmediateWithCarry(regs.reg8[r]);
}

void CPU::AddFromMemAtHLWithCarry() {
    AddImmediateWithCarry(ReadMemAndTick(regs.reg16[HL]));
}

// 8-bit Subtract operations
void CPU::SubImmediate(u8 val) {
    SetHalf((regs.reg8[A] & 0x0F) < (val & 0x0F));
    SetCarry(regs.reg8[A] < val);
    SetSub(true);

    regs.reg8[A] -= val;
    SetZero(!regs.reg8[A]);
}

void CPU::Sub(Reg8Addr r) {
    SubImmediate(regs.reg8[r]);
}

void CPU::SubFromMemAtHL() {
    SubImmediate(ReadMemAndTick(regs.reg16[HL]));
}

void CPU::SubImmediateWithCarry(u8 val) {
    u8 carry_val = Carry();
    SetHalf((regs.reg8[A] & 0x0F) < (val & 0x0F) + carry_val);
    SetCarry(regs.reg8[A] < val + carry_val);
    SetSub(true);

    regs.reg8[A] -= val + carry_val;
    SetZero(!regs.reg8[A]);
}

void CPU::SubWithCarry(Reg8Addr r) {
    SubImmediateWithCarry(regs.reg8[r]);
}

void CPU::SubFromMemAtHLWithCarry() {
    SubImmediateWithCarry(ReadMemAndTick(regs.reg16[HL]));
}

void CPU::IncReg8(Reg8Addr r) {
    SetHalf((regs.reg8[r] & 0x0F) == 0x0F);
    ++regs.reg8[r];
    SetZero(!regs.reg8[r]);
    SetSub(false);
}

void CPU::IncMemAtHL() {
    u8 val = ReadMemAndTick(regs.reg16[HL]);

    SetHalf((val & 0x0F) == 0x0F);
    ++val;
    SetZero(!val);
    SetSub(false);

    WriteMemAndTick(regs.reg16[HL], val);
}

void CPU::DecReg8(Reg8Addr r) {
    SetHalf((regs.reg8[r] & 0x0F) == 0x00);
    --regs.reg8[r];
    SetZero(!regs.reg8[r]);
    SetSub(true);
}

void CPU::DecMemAtHL() {
    u8 val = ReadMemAndTick(regs.reg16[HL]);

    SetHalf((val & 0x0F) == 0x00);
    --val;
    SetZero(!val);
    SetSub(true);

    WriteMemAndTick(regs.reg16[HL], val);
}

// Logical operations
void CPU::AndImmediate(u8 val) {
    regs.reg8[A] &= val;

    SetZero(!regs.reg8[A]);
    SetSub(false);
    SetHalf(true);
    SetCarry(false);
}

void CPU::And(Reg8Addr r) {
    AndImmediate(regs.reg8[r]);
}

void CPU::AndFromMemAtHL() {
    AndImmediate(ReadMemAndTick(regs.reg16[HL]));
}

// Bitwise Or operations
void CPU::OrImmediate(u8 val) {
    regs.reg8[A] |= val;

    SetZero(!regs.reg8[A]);
    SetSub(false);
    SetHalf(false);
    SetCarry(false);
}

void CPU::Or(Reg8Addr r) {
    OrImmediate(regs.reg8[r]);
}

void CPU::OrFromMemAtHL() {
    OrImmediate(ReadMemAndTick(regs.reg16[HL]));
}

// Bitwise Xor operations
void CPU::XorImmediate(u8 val) {
    regs.reg8[A] ^= val;

    SetZero(!regs.reg8[A]);
    SetSub(false);
    SetHalf(false);
    SetCarry(false);
}

void CPU::Xor(Reg8Addr r) {
    XorImmediate(regs.reg8[r]);
}

void CPU::XorFromMemAtHL() {
    XorImmediate(ReadMemAndTick(regs.reg16[HL]));
}

// Compare operations
void CPU::CompareImmediate(u8 val) {
    SetZero(!(regs.reg8[A] - val));
    SetSub(true);
    SetHalf((regs.reg8[A] & 0x0F) < (val & 0x0F));
    SetCarry(regs.reg8[A] < val);
}

void CPU::Compare(Reg8Addr r) {
    CompareImmediate(regs.reg8[r]);
}

void CPU::CompareFromMemAtHL() {
    CompareImmediate(ReadMemAndTick(regs.reg16[HL]));
}

// 16-bit Arithmetic operations
void CPU::AddHL(Reg16Addr r) {
    SetSub(false);
    SetHalf(((regs.reg16[HL] & 0x0FFF) + (regs.reg16[r] & 0x0FFF)) & 0x1000);
    SetCarry((regs.reg16[HL] + regs.reg16[r]) & 0x10000);
    regs.reg16[HL] += regs.reg16[r];

    gameboy->HardwareTick(4);
}

void CPU::AddSP(s8 val) {
    // The half carry & carry flags for this instruction are set by adding the value as an *unsigned* byte to the
    // lower byte of sp. The addition itself is done with the value as a signed byte.
    SetZero(false);
    SetSub(false);

    SetHalf(((regs.reg16[SP] & 0x000F) + (static_cast<u8>(val) & 0x0F)) & 0x0010);
    SetCarry(((regs.reg16[SP] & 0x00FF) + static_cast<u8>(val)) & 0x0100);

    regs.reg16[SP] += val;

    // Two internal delays.
    gameboy->HardwareTick(8);
}

void CPU::IncReg16(Reg16Addr r) {
    ++regs.reg16[r];
    gameboy->HardwareTick(4);
}

void CPU::DecReg16(Reg16Addr r) {
    --regs.reg16[r];
    gameboy->HardwareTick(4);
}

// Miscellaneous arithmetic
void CPU::DecimalAdjustA() {
    if (Sub()) {
        if (Carry()) {
            regs.reg8[A] -= 0x60;
        }
        if (Half()) {
            regs.reg8[A] -= 0x06;
        }
    } else {
        if (Carry() || regs.reg8[A] > 0x99) {
            regs.reg8[A] += 0x60;
            SetCarry(true);
        }
        if (Half() || (regs.reg8[A] & 0x0F) > 0x09) {
            regs.reg8[A] += 0x06;
        }
    }
    SetZero(!regs.reg8[A]);
    SetHalf(false);
}

void CPU::ComplementA() {
    regs.reg8[A] = ~regs.reg8[A];
    SetSub(true);
    SetHalf(true);
}

void CPU::SetCarry() {
    SetCarry(true);
    SetSub(false);
    SetHalf(false);
}

void CPU::ComplementCarry() {
    SetCarry(!Carry());
    SetSub(false);
    SetHalf(false);
}

// Rotates and Shifts
void CPU::RotateLeft(Reg8Addr r) {
    SetCarry(regs.reg8[r] & 0x80);

    regs.reg8[r] = (regs.reg8[r] << 1) | (regs.reg8[r] >> 7);

    SetZero(!regs.reg8[r]);
    SetSub(false);
    SetHalf(false);
}

void CPU::RotateLeftMemAtHL() {
    u8 val = ReadMemAndTick(regs.reg16[HL]);
    SetCarry(val & 0x80);

    val = (val << 1) | (val >> 7);

    SetZero(!val);
    SetSub(false);
    SetHalf(false);

    WriteMemAndTick(regs.reg16[HL], val);
}

void CPU::RotateLeftThroughCarry(Reg8Addr r) {
    u8 carry_val = regs.reg8[r] & 0x80;
    regs.reg8[r] = (regs.reg8[r] << 1) | Carry();

    SetZero(!regs.reg8[r]);
    SetSub(false);
    SetHalf(false);
    SetCarry(carry_val);
}

void CPU::RotateLeftMemAtHLThroughCarry() {
    u8 val = ReadMemAndTick(regs.reg16[HL]);
    u8 carry_val = val & 0x80;

    val = (val << 1) | Carry();

    SetZero(!val);
    SetSub(false);
    SetHalf(false);
    SetCarry(carry_val);

    WriteMemAndTick(regs.reg16[HL], val);
}

void CPU::RotateRight(Reg8Addr r) {
    SetCarry(regs.reg8[r] & 0x01);

    regs.reg8[r] = (regs.reg8[r] >> 1) | (regs.reg8[r] << 7);

    SetZero(!regs.reg8[r]);
    SetSub(false);
    SetHalf(false);
}

void CPU::RotateRightMemAtHL() {
    u8 val = ReadMemAndTick(regs.reg16[HL]);
    SetCarry(val & 0x01);

    val = (val >> 1) | (val << 7);

    SetZero(!val);
    SetSub(false);
    SetHalf(false);

    WriteMemAndTick(regs.reg16[HL], val);
}

void CPU::RotateRightThroughCarry(Reg8Addr r) {
    u8 carry_val = regs.reg8[r] & 0x01;
    regs.reg8[r] = (regs.reg8[r] >> 1) | (Carry() << 7);

    SetZero(!regs.reg8[r]);
    SetSub(false);
    SetHalf(false);
    SetCarry(carry_val);
}

void CPU::RotateRightMemAtHLThroughCarry() {
    u8 val = ReadMemAndTick(regs.reg16[HL]);
    u8 carry_val = val & 0x01;

    val = (val >> 1) | (Carry() << 7);

    SetZero(!val);
    SetSub(false);
    SetHalf(false);
    SetCarry(carry_val);

    WriteMemAndTick(regs.reg16[HL], val);
}

void CPU::ShiftLeft(Reg8Addr r) {
    SetCarry(regs.reg8[r] & 0x80);

    regs.reg8[r] <<= 1;

    SetZero(!regs.reg8[r]);
    SetSub(false);
    SetHalf(false);
}

void CPU::ShiftLeftMemAtHL() {
    u8 val = ReadMemAndTick(regs.reg16[HL]);
    SetCarry(val & 0x80);

    val <<= 1;

    SetZero(!val);
    SetSub(false);
    SetHalf(false);

    WriteMemAndTick(regs.reg16[HL], val);
}

void CPU::ShiftRightArithmetic(Reg8Addr r) {
    SetCarry(regs.reg8[r] & 0x01);

    regs.reg8[r] >>= 1;
    regs.reg8[r] |= (regs.reg8[r] & 0x40) << 1;

    SetZero(!regs.reg8[r]);
    SetSub(false);
    SetHalf(false);
}

void CPU::ShiftRightArithmeticMemAtHL() {
    u8 val = ReadMemAndTick(regs.reg16[HL]);
    SetCarry(val & 0x01);

    val >>= 1;
    val |= (val & 0x40) << 1;

    SetZero(!val);
    SetSub(false);
    SetHalf(false);

    WriteMemAndTick(regs.reg16[HL], val);
}

void CPU::ShiftRightLogical(Reg8Addr r) {
    SetCarry(regs.reg8[r] & 0x01);

    regs.reg8[r] >>= 1;

    SetZero(!regs.reg8[r]);
    SetSub(false);
    SetHalf(false);
}

void CPU::ShiftRightLogicalMemAtHL() {
    u8 val = ReadMemAndTick(regs.reg16[HL]);
    SetCarry(val & 0x01);

    val >>= 1;

    SetZero(!val);
    SetSub(false);
    SetHalf(false);

    WriteMemAndTick(regs.reg16[HL], val);
}

void CPU::SwapNybbles(Reg8Addr r) {
    regs.reg8[r] = (regs.reg8[r] << 4) | (regs.reg8[r] >> 4);

    SetZero(!regs.reg8[r]);
    SetSub(false);
    SetHalf(false);
    SetCarry(false);
}

void CPU::SwapMemAtHL() {
    u8 val = ReadMemAndTick(regs.reg16[HL]);

    val = (val << 4) | (val >> 4);

    SetZero(!val);
    SetSub(false);
    SetHalf(false);
    SetCarry(false);

    WriteMemAndTick(regs.reg16[HL], val);
}

// Bit manipulation
void CPU::TestBit(unsigned int bit, Reg8Addr r) {
    SetZero(!(regs.reg8[r] & (0x01 << bit)));
    SetSub(false);
    SetHalf(true);
}

void CPU::TestBitOfMemAtHL(unsigned int bit) {
    SetZero(!(ReadMemAndTick(regs.reg16[HL]) & (0x01 << bit)));
    SetSub(false);
    SetHalf(true);
}

void CPU::ResetBit(unsigned int bit, Reg8Addr r) {
    regs.reg8[r] &= ~(0x01 << bit);
}

void CPU::ResetBitOfMemAtHL(unsigned int bit) {
    u8 val = ReadMemAndTick(regs.reg16[HL]);

    val &= ~(0x01 << bit);

    WriteMemAndTick(regs.reg16[HL], val);
}

void CPU::SetBit(unsigned int bit, Reg8Addr r) {
    regs.reg8[r] |= (0x01 << bit);
}

void CPU::SetBitOfMemAtHL(unsigned int bit) {
    u8 val = ReadMemAndTick(regs.reg16[HL]);

    val |= (0x01 << bit);

    WriteMemAndTick(regs.reg16[HL], val);
}

// Jumps
void CPU::Jump(u16 addr) {
    // Internal delay
    gameboy->HardwareTick(4);

    pc = addr;
}

void CPU::JumpToHL() {
    pc = regs.reg16[HL];
}

void CPU::RelativeJump(s8 val) {
    // Internal delay
    gameboy->HardwareTick(4);

    pc += val;
}

// Calls and Returns
void CPU::Call(u16 addr) {
    // Internal delay
    gameboy->HardwareTick(4);

    WriteMemAndTick(--regs.reg16[SP], static_cast<u8>(pc >> 8));
    WriteMemAndTick(--regs.reg16[SP], static_cast<u8>(pc));

    pc = addr;
}

void CPU::Return() {
    u8 byte_lo = ReadMemAndTick(regs.reg16[SP]++);
    u8 byte_hi = ReadMemAndTick(regs.reg16[SP]++);

    pc = (static_cast<u16>(byte_hi) << 8) | static_cast<u16>(byte_lo);

    // Internal delay
    gameboy->HardwareTick(4);
}

// System Control
void CPU::Halt() {
    if (!interrupt_master_enable && mem.RequestedEnabledInterrupts()) {
        // If interrupts are disabled and there are requested, enabled interrupts pending when HALT is executed,
        // the GB will not enter halt mode. Instead, the GB will fail to increase the PC when executing the next 
        // instruction, thus executing it twice.
        cpu_mode = CPUMode::HaltBug;
    } else {
        cpu_mode = CPUMode::Halted;
    }
}

void CPU::Stop() {
    // STOP is a two-byte long opcode. If the opcode following STOP is not 0x00, the LCD supposedly turns on?
    ++pc;
    gameboy->HaltedTick(4);

    // Turn off the LCD.
    gameboy->StopLCD();

    // During STOP mode, the clock increases as usual, but normal interrupts are not serviced or checked. Regardless
    // if the joypad interrupt is enabled in the IE register, a stopped Game Boy will intercept any joypad presses
    // if the corresponding input lines in the P1 register are enabled.

    // Check if we should begin a speed switch.
    if (mem.game_mode == GameMode::CGB && mem.ReadMem8(0xFF4D) & 0x01) {
        // A speed switch takes 128*1024-80=130992 cycles to complete, plus 4 cycles to decode the STOP instruction.
        speed_switch_cycles = 130992;
    } else if ((mem.ReadMem8(0xFF00) & 0x30) == 0x30) {
        throw std::runtime_error("The CPU has hung. Reason: STOP mode was entered with all joypad inputs disabled.");
    }

    cpu_mode = CPUMode::Stopped;
}

} // End namespace Gb
