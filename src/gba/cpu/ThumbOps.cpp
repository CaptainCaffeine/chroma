// This file is a part of Chroma.
// Copyright (C) 2017 Matthew Murray
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

#include <bitset>

#include "gba/cpu/Cpu.h"
#include "gba/memory/Memory.h"

namespace Gba {

// Arithmetic Operators
int Cpu::Thumb_AdcReg(Reg m, Reg d) {
    u64 result = AddWithCarry(regs[d], regs[m], GetCarry());

    regs[d] = result;
    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_AddImmT1(u32 imm, Reg n, Reg d) {
    u64 result = AddWithCarry(regs[n], imm, 0);

    regs[d] = result;
    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_AddImmT2(Reg d, u32 imm) {
    u64 result = AddWithCarry(regs[d], imm, 0);

    regs[d] = result;
    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_AddRegT1(Reg m, Reg n, Reg d) {
    u64 result = AddWithCarry(regs[n], regs[m], 0);

    regs[d] = result;
    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_AddRegT2(Reg d1, Reg m, Reg d2) {
    Reg d = (d1 << 3) | d2;

    // At least one of Rd or Rm must be from >R8.
    assert(d >= 8 || m >= 8);
    assert(d != pc || m != pc);

    u64 result = AddWithCarry(regs[d], regs[m], 0);

    if (d == pc) {
        Thumb_BranchWritePC(result);
    } else {
        regs[d] = result;
        // Don't set flags.
    }

    return 1;
}

int Cpu::Thumb_AddSpImmT1(Reg d, u32 imm) {
    imm <<= 2;

    u64 result = AddWithCarry(regs[sp], imm, 0);

    regs[d] = result;
    // Don't set flags.

    return 1;
}

int Cpu::Thumb_AddSpImmT2(u32 imm) {
    imm <<= 2;

    u64 result = AddWithCarry(regs[sp], imm, 0);

    regs[sp] = result;
    // Don't set flags.

    return 1;
}

int Cpu::Thumb_AddPcImm(Reg d, u32 imm) {
    imm <<= 2;

    u64 result = AddWithCarry(regs[pc] & ~0x3, imm, 0);

    regs[d] = result;
    // Don't set flags.

    return 1;
}

int Cpu::Thumb_CmnReg(Reg m, Reg n) {
    u64 result = AddWithCarry(regs[n], regs[m], 0);

    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_CmpImm(Reg n, u32 imm) {
    u64 result = AddWithCarry(regs[n], ~imm, 1);

    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_CmpRegT1(Reg m, Reg n) {
    u64 result = AddWithCarry(regs[n], ~regs[m], 1);

    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_CmpRegT2(Reg n1, Reg m, Reg n2) {
    Reg n = (n1 << 3) | n2;

    // At least one of Rn or Rm must be from R8-R14.
    assert(n >= 8 || m >= 8);
    assert(n != pc && m != pc);

    u64 result = AddWithCarry(regs[n], ~regs[m], 1);
    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_MulReg(Reg n, Reg d) {
    assert(d != n); // Unpredictable

    u32 result = regs[d] * regs[n];

    regs[d] = result;
    // The carry flag gets destroyed on ARMv4.
    SetSignZeroCarryFlags(result, 0);

    return 1;
}

int Cpu::Thumb_RsbImm(Reg n, Reg d) {
    // The immediate is always 0 for this instruction.
    u64 result = AddWithCarry(~regs[n], 0, 1);

    regs[d] = result;
    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_SbcReg(Reg m, Reg d) {
    u64 result = AddWithCarry(regs[d], ~regs[m], GetCarry());

    regs[d] = result;
    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_SubImmT1(u32 imm, Reg n, Reg d) {
    u64 result = AddWithCarry(regs[n], ~imm, 1);

    regs[d] = result;
    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_SubImmT2(Reg d, u32 imm) {
    u64 result = AddWithCarry(regs[d], ~imm, 1);

    regs[d] = result;
    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_SubReg(Reg m, Reg n, Reg d) {
    u64 result = AddWithCarry(regs[n], ~regs[m], 1);

    regs[d] = result;
    SetAllFlags(result);

    return 1;
}

int Cpu::Thumb_SubSpImm(u32 imm) {
    imm <<= 2;

    u64 result = AddWithCarry(regs[sp], ~imm, 1);

    regs[sp] = result;
    // Don't set flags.

    return 1;
}

// Logical Operators
int Cpu::Thumb_AndReg(Reg m, Reg d) {
    u32 result = regs[d] & regs[m];

    regs[d] = result;
    SetSignZeroFlags(result);

    return 1;
}

int Cpu::Thumb_BicReg(Reg m, Reg d) {
    u32 result = regs[d] & ~regs[m];

    regs[d] = result;
    SetSignZeroFlags(result);

    return 1;
}

int Cpu::Thumb_EorReg(Reg m, Reg d) {
    u32 result = regs[d] ^ regs[m];

    regs[d] = result;
    SetSignZeroFlags(result);

    return 1;
}

int Cpu::Thumb_OrrReg(Reg m, Reg d) {
    u32 result = regs[d] | regs[m];

    regs[d] = result;
    SetSignZeroFlags(result);

    return 1;
}

int Cpu::Thumb_TstReg(Reg m, Reg n) {
    u32 result = regs[n] & regs[m];

    SetSignZeroFlags(result);

    return 1;
}

// Shifts
int Cpu::Thumb_AsrImm(u32 imm, Reg m, Reg d) {
    ImmediateShift shift = DecodeImmShift(ShiftType::ASR, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], ShiftType::ASR, shift.imm, GetCarry());

    regs[d] = shifted_reg.result;
    SetSignZeroCarryFlags(shifted_reg.result, shifted_reg.carry);

    return 1;
}

int Cpu::Thumb_AsrReg(Reg m, Reg d) {
    ResultWithCarry shifted_reg = Shift_C(regs[d], ShiftType::ASR, regs[m] & 0xFF, GetCarry());

    regs[d] = shifted_reg.result;
    SetSignZeroCarryFlags(shifted_reg.result, shifted_reg.carry);

    return 1;
}

int Cpu::Thumb_LslImm(u32 imm, Reg m, Reg d) {
    // No need to DecodeImmShift for LSL.

    ResultWithCarry shifted_reg = Shift_C(regs[m], ShiftType::LSL, imm, GetCarry());

    regs[d] = shifted_reg.result;
    SetSignZeroCarryFlags(shifted_reg.result, shifted_reg.carry);

    return 1;
}

int Cpu::Thumb_LslReg(Reg m, Reg d) {
    ResultWithCarry shifted_reg = Shift_C(regs[d], ShiftType::LSL, regs[m] & 0xFF, GetCarry());

    regs[d] = shifted_reg.result;
    SetSignZeroCarryFlags(shifted_reg.result, shifted_reg.carry);

    return 1;
}

int Cpu::Thumb_LsrImm(u32 imm, Reg m, Reg d) {
    ImmediateShift shift = DecodeImmShift(ShiftType::LSR, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], ShiftType::LSR, shift.imm, GetCarry());

    regs[d] = shifted_reg.result;
    SetSignZeroCarryFlags(shifted_reg.result, shifted_reg.carry);

    return 1;
}

int Cpu::Thumb_LsrReg(Reg m, Reg d) {
    ResultWithCarry shifted_reg = Shift_C(regs[d], ShiftType::LSR, regs[m] & 0xFF, GetCarry());

    regs[d] = shifted_reg.result;
    SetSignZeroCarryFlags(shifted_reg.result, shifted_reg.carry);

    return 1;
}

int Cpu::Thumb_RorReg(Reg m, Reg d) {
    ResultWithCarry shifted_reg = Shift_C(regs[d], ShiftType::ROR, regs[m] & 0xFF, GetCarry());

    regs[d] = shifted_reg.result;
    SetSignZeroCarryFlags(shifted_reg.result, shifted_reg.carry);

    return 1;
}

// Branches
int Cpu::Thumb_BT1(Condition cond, u32 imm8) {
    assert(cond != Condition::Always); // Undefined

    if (!ConditionPassed(cond)) {
        return 1;
    }

    s32 signed_imm32 = SignExtend(imm8 << 1, 9);

    Thumb_BranchWritePC(regs[pc] + signed_imm32);

    return 1;
}

int Cpu::Thumb_BT2(u32 imm11) {
    s32 signed_imm32 = SignExtend(imm11 << 1, 12);

    Thumb_BranchWritePC(regs[pc] + signed_imm32);

    return 1;
}

int Cpu::Thumb_BlH1(u32 imm11) {
    // The next instruction must be BlH2.
    assert((mem.ReadMem<u32>(regs[pc] - 2) & 0xF800) == 0xF800);

    s32 signed_imm32 = SignExtend(imm11 << 12, 23);

    regs[lr] = regs[pc] + signed_imm32;

    return 1;
}

int Cpu::Thumb_BlH2(u32 imm11) {
    // The previous instruction must be BlH1.
    assert((mem.ReadMem<u32>(regs[pc] - 6) & 0xF800) == 0xF000);

    u32 next_instr_addr = regs[pc] - 2;
    Thumb_BranchWritePC(regs[lr] + (imm11 << 1));
    regs[lr] = next_instr_addr | 0x1;

    return 1;
}

int Cpu::Thumb_Bx(Reg m) {
    BxWritePC(regs[m]);

    return 1;
}

// Moves
int Cpu::Thumb_MovImm(Reg d, u32 imm) {
    regs[d] = imm;
    SetSignZeroFlags(imm);

    return 1;
}

int Cpu::Thumb_MovRegT1(Reg d1, Reg m, Reg d2) {
    Reg d = (d1 << 3) | d2;

    // At least one of Rd or Rm must be from >R8.
    assert(d >= 8 || m >= 8);

    if (d == pc) {
        Thumb_BranchWritePC(regs[m]);
    } else {
        regs[d] = regs[m];
        // Don't set flags.
    }

    return 1;
}

int Cpu::Thumb_MovRegT2(Reg m, Reg d) {
    regs[d] = regs[m];
    SetSignZeroFlags(regs[m]);

    return 1;
}

int Cpu::Thumb_MvnReg(Reg m, Reg d) {
    regs[d] = ~regs[m];
    SetSignZeroFlags(~regs[m]);

    return 1;
}

// Loads
int Cpu::Thumb_Ldm(Reg n, u32 reg_list) {
    assert(Popcount(reg_list) != 0); // Unpredictable

    const std::bitset<8> rlist{reg_list};
    u32 addr = regs[n];

    for (Reg i = 0; i < 8; ++i) {
        if (rlist[i]) {
            // Reads must be aligned.
            regs[i] = mem.ReadMem<u32>(addr & ~0x3);
            addr += 4;
        }
    }

    // Only write back to Rn if it wasn't in the register list.
    if (!rlist[n]) {
        regs[n] = addr;
    }

    return 1;
}

int Cpu::Thumb_LdrImm(u32 imm, Reg n, Reg t) {
    imm <<= 2;

    u32 addr = regs[n] + imm;
    regs[t] = mem.ReadMem<u32>(addr);

    return 1;
}

int Cpu::Thumb_LdrSpImm(Reg t, u32 imm) {
    imm <<= 2;

    u32 addr = regs[sp] + imm;
    regs[t] = mem.ReadMem<u32>(addr);

    return 1;
}

int Cpu::Thumb_LdrPcImm(Reg t, u32 imm) {
    imm <<= 2;

    u32 addr = (regs[pc] & ~0x3) + imm;
    regs[t] = mem.ReadMem<u32>(addr);

    return 1;
}

int Cpu::Thumb_LdrReg(Reg m, Reg n, Reg t) {
    u32 addr = regs[n] + regs[m];
    regs[t] = mem.ReadMem<u32>(addr);

    return 1;
}

int Cpu::Thumb_LdrbImm(u32 imm, Reg n, Reg t) {
    u32 addr = regs[n] + imm;
    regs[t] = mem.ReadMem<u8>(addr);

    return 1;
}

int Cpu::Thumb_LdrbReg(Reg m, Reg n, Reg t) {
    u32 addr = regs[n] + regs[m];
    regs[t] = mem.ReadMem<u8>(addr);

    return 1;
}

int Cpu::Thumb_LdrhImm(u32 imm, Reg n, Reg t) {
    imm <<= 1;

    u32 addr = regs[n] + imm;
    regs[t] = mem.ReadMem<u16>(addr);

    return 1;
}

int Cpu::Thumb_LdrhReg(Reg m, Reg n, Reg t) {
    u32 addr = regs[n] + regs[m];
    regs[t] = mem.ReadMem<u16>(addr);

    return 1;
}

int Cpu::Thumb_LdrsbReg(Reg m, Reg n, Reg t) {
    u32 addr = regs[n] + regs[m];
    regs[t] = SignExtend(static_cast<u32>(mem.ReadMem<u8>(addr)), 8);

    return 1;
}

int Cpu::Thumb_LdrshReg(Reg m, Reg n, Reg t) {
    u32 addr = regs[n] + regs[m];
    regs[t] = SignExtend(static_cast<u32>(mem.ReadMem<u16>(addr)), 16);

    return 1;
}

int Cpu::Thumb_Pop(bool p, u32 reg_list) {
    assert(Popcount(reg_list) != 0); // Unpredictable

    const std::bitset<8> rlist{reg_list};
    u32 addr = regs[sp];

    for (Reg i = 0; i < 8; ++i) {
        if (rlist[i]) {
            // Reads must be aligned.
            regs[i] = mem.ReadMem<u32>(addr & ~0x3);
            addr += 4;
        }
    }

    if (p) {
        Thumb_BranchWritePC(mem.ReadMem<u32>(addr & ~0x3));
        addr += 4;
    }

    regs[sp] = addr;

    return 1;
}

// Stores
int Cpu::Thumb_Push(bool m, u32 reg_list) {
    assert(Popcount(reg_list) != 0); // Unpredictable

    const std::bitset<8> rlist{reg_list};
    regs[sp] -= 4 * rlist.count();
    u32 addr = regs[sp];

    for (Reg i = 0; i < 8; ++i) {
        if (rlist[i]) {
            // Writes are always aligned.
            mem.WriteMem(addr, regs[i]);
            addr += 4;
        }
    }

    if (m) {
        mem.WriteMem(addr, regs[lr]);
    }

    return 1;
}

int Cpu::Thumb_Stm(Reg n, u32 reg_list) {
    assert(Popcount(reg_list) != 0); // Unpredictable

    const std::bitset<8> rlist{reg_list};
    u32 addr = regs[n];

    for (Reg i = 0; i < 8; ++i) {
        if (rlist[i]) {
            // Writes are always aligned.
            if (i == n && n != LowestSetBit(reg_list)) {
                // Store the new Rn value if it's not the first register in the list.
                mem.WriteMem(addr, static_cast<u32>(regs[n] + 4 * rlist.count()));
            } else {
                mem.WriteMem(addr, regs[i]);
            }
            addr += 4;
        }
    }

    regs[n] = addr;

    return 1;
}

int Cpu::Thumb_StrImm(u32 imm, Reg n, Reg t) {
    imm <<= 2;

    u32 addr = regs[n] + imm;
    mem.WriteMem(addr, regs[t]);

    return 1;
}

int Cpu::Thumb_StrSpImm(Reg t, u32 imm) {
    imm <<= 2;

    u32 addr = regs[sp] + imm;
    mem.WriteMem(addr, regs[t]);

    return 1;
}

int Cpu::Thumb_StrReg(Reg m, Reg n, Reg t) {
    u32 addr = regs[n] + regs[m];
    mem.WriteMem(addr, regs[t]);

    return 1;
}

int Cpu::Thumb_StrbImm(u32 imm, Reg n, Reg t) {
    u32 addr = regs[n] + imm;
    mem.WriteMem(addr, static_cast<u8>(regs[t]));

    return 1;
}

int Cpu::Thumb_StrbReg(Reg m, Reg n, Reg t) {
    u32 addr = regs[n] + regs[m];
    mem.WriteMem(addr, static_cast<u8>(regs[t]));

    return 1;
}

int Cpu::Thumb_StrhImm(u32 imm, Reg n, Reg t) {
    imm <<= 1;

    u32 addr = regs[n] + imm;
    mem.WriteMem(addr, static_cast<u16>(regs[t]));

    return 1;
}

int Cpu::Thumb_StrhReg(Reg m, Reg n, Reg t) {
    u32 addr = regs[n] + regs[m];
    mem.WriteMem(addr, static_cast<u16>(regs[t]));

    return 1;
}

// Misc
int Cpu::Thumb_Swi(u32) {
    assert(false && "Exceptions unimplemented");

    return 1;
}

int Cpu::Thumb_Undefined(u16) {
    assert(false && "Exceptions unimplemented");

    return 1;
}

} // End namespace Gba
