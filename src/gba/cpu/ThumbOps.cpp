// This file is a part of Chroma.
// Copyright (C) 2017-2018 Matthew Murray
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
#include <cassert>

#include "gba/cpu/Cpu.h"
#include "gba/memory/Memory.h"

namespace Gba {

int Cpu::Thumb_ArithImm(u32 imm, Reg n, Reg d, ArithOp op, u32 carry) {
    ArithResult result = op(regs[n], imm, carry);

    regs[d] = result.value;
    SetAllFlags(result);

    return 0;
}

int Cpu::Thumb_ArithReg(Reg m, Reg n, Reg d, ArithOp op, u32 carry) {
    ArithResult result = op(regs[n], regs[m], carry);

    regs[d] = result.value;
    SetAllFlags(result);

    return 0;
}

int Cpu::Thumb_ArithImmSp(Reg d, u32 imm, ArithOp op, u32 carry) {
    imm <<= 2;

    u64 result = op(regs[sp], imm, carry).value;

    regs[d] = result;
    // Don't set flags.

    return 0;
}

int Cpu::Thumb_Compare(u32 imm, Reg n, ArithOp op, u32 carry) {
    ArithResult result = op(regs[n], imm, carry);

    SetAllFlags(result);

    return 0;
}

int Cpu::Thumb_LogicReg(Reg m, Reg d, LogicOp op) {
    u32 result = op(regs[d], regs[m]);

    regs[d] = result;
    SetSignZeroFlags(result);

    return 0;
}

int Cpu::Thumb_ShiftImm(u32 imm, Reg m, Reg d, ShiftType type) {
    ImmediateShift shift = DecodeImmShift(type, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], shift.type, shift.imm);

    regs[d] = shifted_reg.result;
    SetSignZeroCarryFlags(shifted_reg.result, shifted_reg.carry);

    return 0;
}

int Cpu::Thumb_ShiftReg(Reg m, Reg d, ShiftType type) {
    ResultWithCarry shifted_reg = Shift_C(regs[d], type, regs[m] & 0xFF);

    regs[d] = shifted_reg.result;
    SetSignZeroCarryFlags(shifted_reg.result, shifted_reg.carry);

    // One internal cycle for shifting by register.
    InternalCycle(1);

    return 1;
}

int Cpu::Thumb_Load(u32 imm, Reg n, Reg t, LoadOp op) {
    u32 addr = regs[n] + imm;

    int cycles;
    std::tie(regs[t], cycles) = op(mem, addr);

    // Plus one internal cycle to transfer the loaded value to Rt.
    cycles += 1;
    LoadInternalCycle(1);

    return cycles;
}

int Cpu::Thumb_Store(u32 imm, Reg n, Reg t, StoreOp op) {
    u32 addr = regs[n] + imm;
    return op(mem, addr, regs[t]);
}

// Arithmetic Operators
int Cpu::Thumb_AdcReg(Reg m, Reg d) {
    return Thumb_ArithReg(m, d, d, add_op, GetCarry());
}

int Cpu::Thumb_AddImmT1(u32 imm, Reg n, Reg d) {
    return Thumb_ArithImm(imm, n, d, add_op, 0);
}

int Cpu::Thumb_AddImmT2(Reg d, u32 imm) {
    return Thumb_ArithImm(imm, d, d, add_op, 0);
}

int Cpu::Thumb_AddRegT1(Reg m, Reg n, Reg d) {
    return Thumb_ArithReg(m, n, d, add_op, 0);
}

int Cpu::Thumb_AddRegT2(Reg d1, Reg m, Reg d2) {
    Reg d = (d1 << 3) | d2;

    // At least one of Rd or Rm must be from >R8.
    assert(d >= 8 || m >= 8);
    assert(d != pc || m != pc);

    u64 result = AddWithCarry(regs[d], regs[m], 0).value;

    if (d == pc) {
        return Thumb_BranchWritePC(result);
    } else {
        regs[d] = result;
        // Don't set flags.
    }

    return 0;
}

int Cpu::Thumb_AddSpImmT1(Reg d, u32 imm) {
    return Thumb_ArithImmSp(d, imm, add_op, 0);
}

int Cpu::Thumb_AddSpImmT2(u32 imm) {
    return Thumb_ArithImmSp(sp, imm, add_op, 0);
}

int Cpu::Thumb_AddPcImm(Reg d, u32 imm) {
    imm <<= 2;

    u64 result = AddWithCarry(regs[pc] & ~0x3, imm, 0).value;

    regs[d] = result;
    // Don't set flags.

    return 0;
}

int Cpu::Thumb_CmnReg(Reg m, Reg n) {
    return Thumb_Compare(regs[m], n, add_op, 0);
}

int Cpu::Thumb_CmpImm(Reg n, u32 imm) {
    return Thumb_Compare(imm, n, sub_op, 1);
}

int Cpu::Thumb_CmpRegT1(Reg m, Reg n) {
    return Thumb_Compare(regs[m], n, sub_op, 1);
}

int Cpu::Thumb_CmpRegT2(Reg n1, Reg m, Reg n2) {
    Reg n = (n1 << 3) | n2;

    // At least one of Rn or Rm must be from R8-R14.
    assert(n >= 8 || m >= 8);
    assert(n != pc && m != pc);

    ArithResult result = AddWithCarry(regs[n], ~regs[m], 1);
    SetAllFlags(result);

    return 0;
}

int Cpu::Thumb_MulReg(Reg n, Reg d) {
    assert(d != n); // Unpredictable

    int cycles = MultiplyCycles(regs[d]);
    u32 result = regs[d] * regs[n];

    regs[d] = result;
    // The carry flag gets destroyed on ARMv4.
    SetSignZeroCarryFlags(result, 0);

    InternalCycle(cycles);

    return cycles;
}

int Cpu::Thumb_RsbImm(Reg n, Reg d) {
    // The immediate is always 0 for this instruction.
    return Thumb_ArithImm(0, n, d, rsb_op, 1);
}

int Cpu::Thumb_SbcReg(Reg m, Reg d) {
    return Thumb_ArithReg(m, d, d, sub_op, GetCarry());
}

int Cpu::Thumb_SubImmT1(u32 imm, Reg n, Reg d) {
    return Thumb_ArithImm(imm, n, d, sub_op, 1);
}

int Cpu::Thumb_SubImmT2(Reg d, u32 imm) {
    return Thumb_ArithImm(imm, d, d, sub_op, 1);
}

int Cpu::Thumb_SubReg(Reg m, Reg n, Reg d) {
    return Thumb_ArithReg(m, n, d, sub_op, 1);
}

int Cpu::Thumb_SubSpImm(u32 imm) {
    return Thumb_ArithImmSp(sp, imm, sub_op, 1);
}

// Logical Operators
int Cpu::Thumb_AndReg(Reg m, Reg d) {
    return Thumb_LogicReg(m, d, and_op);
}

int Cpu::Thumb_BicReg(Reg m, Reg d) {
    return Thumb_LogicReg(m, d, bic_op);
}

int Cpu::Thumb_EorReg(Reg m, Reg d) {
    return Thumb_LogicReg(m, d, eor_op);
}

int Cpu::Thumb_OrrReg(Reg m, Reg d) {
    return Thumb_LogicReg(m, d, orr_op);
}

int Cpu::Thumb_TstReg(Reg m, Reg n) {
    u32 result = regs[n] & regs[m];

    SetSignZeroFlags(result);

    return 0;
}

// Shifts
int Cpu::Thumb_AsrImm(u32 imm, Reg m, Reg d) {
    return Thumb_ShiftImm(imm, m, d, ShiftType::ASR);
}

int Cpu::Thumb_AsrReg(Reg m, Reg d) {
    return Thumb_ShiftReg(m, d, ShiftType::ASR);
}

int Cpu::Thumb_LslImm(u32 imm, Reg m, Reg d) {
    return Thumb_ShiftImm(imm, m, d, ShiftType::LSL);
}

int Cpu::Thumb_LslReg(Reg m, Reg d) {
    return Thumb_ShiftReg(m, d, ShiftType::LSL);
}

int Cpu::Thumb_LsrImm(u32 imm, Reg m, Reg d) {
    return Thumb_ShiftImm(imm, m, d, ShiftType::LSR);
}

int Cpu::Thumb_LsrReg(Reg m, Reg d) {
    return Thumb_ShiftReg(m, d, ShiftType::LSR);
}

int Cpu::Thumb_RorReg(Reg m, Reg d) {
    return Thumb_ShiftReg(m, d, ShiftType::ROR);
}

// Branches
int Cpu::Thumb_BT1(Condition cond, u32 imm8) {
    if (cond == Condition::Always) {
        return TakeException(CpuMode::Undef);
    }

    if (!ConditionPassed(cond)) {
        return 0;
    }

    s32 signed_imm32 = SignExtend(imm8 << 1, 9);

    return Thumb_BranchWritePC(regs[pc] + signed_imm32);
}

int Cpu::Thumb_BT2(u32 imm11) {
    s32 signed_imm32 = SignExtend(imm11 << 1, 12);

    return Thumb_BranchWritePC(regs[pc] + signed_imm32);
}

int Cpu::Thumb_BlH1(u32 imm11) {
    // Unpredictable if the next instruction is not BlH2. This apparently might not be an issue on the GBA.
    assert((mem.ReadMem<u16>(regs[pc] - 2) & 0xF800) == 0xF800);

    s32 signed_imm32 = SignExtend(imm11 << 12, 23);
    regs[lr] = regs[pc] + signed_imm32;

    return 0;
}

int Cpu::Thumb_BlH2(u32 imm11) {
    // Unpredictable if the previous instruction is not BlH1. This apparently might not be an issue on the GBA.
    assert((mem.ReadMem<u16>(regs[pc] - 6) & 0xF800) == 0xF000);

    u32 next_instr_addr = regs[pc] - 2;
    int cycles = Thumb_BranchWritePC(regs[lr] + (imm11 << 1));
    regs[lr] = next_instr_addr | 0x1;

    return cycles;
}

int Cpu::Thumb_Bx(Reg m) {
    return BxWritePC(regs[m]);
}

// Moves
int Cpu::Thumb_MovImm(Reg d, u32 imm) {
    regs[d] = imm;
    SetSignZeroFlags(imm);

    return 0;
}

int Cpu::Thumb_MovRegT1(Reg d1, Reg m, Reg d2) {
    Reg d = (d1 << 3) | d2;

    // At least one of Rd or Rm must be from >R8.
    assert(d >= 8 || m >= 8);

    if (d == pc) {
        return Thumb_BranchWritePC(regs[m]);
    } else {
        regs[d] = regs[m];
        // Don't set flags.
    }

    return 0;
}

int Cpu::Thumb_MovRegT2(Reg m, Reg d) {
    regs[d] = regs[m];
    SetSignZeroFlags(regs[d]);

    return 0;
}

int Cpu::Thumb_MvnReg(Reg m, Reg d) {
    regs[d] = ~regs[m];
    SetSignZeroFlags(regs[d]);

    return 0;
}

// Loads
int Cpu::Thumb_Ldm(Reg n, u32 reg_list) {
    assert(Popcount(reg_list) != 0); // Unpredictable

    const std::bitset<8> rlist{reg_list};
    u32 addr = regs[n];

    // One internal cycle to transfer the last loaded value to the destination register.
    int cycles = 1;

    for (Reg i = 0; i < 8; ++i) {
        if (rlist[i]) {
            // Reads are aligned.
            regs[i] = mem.ReadMem<u32>(addr);
            cycles += mem.AccessTime<u32>(addr);
            addr += 4;
        }
    }

    // Only write back to Rn if it wasn't in the register list.
    if (!rlist[n]) {
        regs[n] = addr;
    }

    LoadInternalCycle(1);

    return cycles;
}

int Cpu::Thumb_LdrImm(u32 imm, Reg n, Reg t) {
    auto ldr_op = [](Memory& _mem, u32 addr) -> std::tuple<u32, int> {
        return std::make_tuple(RotateRight(_mem.ReadMem<u32>(addr), (addr & 0x3) * 8), _mem.AccessTime<u32>(addr));
    };
    return Thumb_Load(imm << 2, n, t, ldr_op);
}

int Cpu::Thumb_LdrSpImm(Reg t, u32 imm) {
    auto ldr_op = [](Memory& _mem, u32 addr) -> std::tuple<u32, int> {
        return std::make_tuple(RotateRight(_mem.ReadMem<u32>(addr), (addr & 0x3) * 8), _mem.AccessTime<u32>(addr));
    };
    return Thumb_Load(imm << 2, sp, t, ldr_op);
}

int Cpu::Thumb_LdrPcImm(Reg t, u32 imm) {
    imm <<= 2;

    u32 addr = (regs[pc] & ~0x3) + imm;
    // No need for a rotation, addr is always 4-byte aligned.
    regs[t] = mem.ReadMem<u32>(addr);
    // Plus one internal cycle to transfer the loaded value to Rt.
    int cycles = 1 + mem.AccessTime<u32>(addr);

    LoadInternalCycle(1);

    return cycles;
}

int Cpu::Thumb_LdrReg(Reg m, Reg n, Reg t) {
    auto ldr_op = [](Memory& _mem, u32 addr) -> std::tuple<u32, int> {
        return std::make_tuple(RotateRight(_mem.ReadMem<u32>(addr), (addr & 0x3) * 8), _mem.AccessTime<u32>(addr));
    };
    return Thumb_Load(regs[m], n, t, ldr_op);
}

int Cpu::Thumb_LdrbImm(u32 imm, Reg n, Reg t) {
    auto ldrb_op = [](Memory& _mem, u32 addr) -> std::tuple<u32, int> {
        return std::make_tuple(_mem.ReadMem<u8>(addr), _mem.AccessTime<u8>(addr));
    };
    return Thumb_Load(imm, n, t, ldrb_op);
}

int Cpu::Thumb_LdrbReg(Reg m, Reg n, Reg t) {
    auto ldrb_op = [](Memory& _mem, u32 addr) -> std::tuple<u32, int> {
        return std::make_tuple(_mem.ReadMem<u8>(addr), _mem.AccessTime<u8>(addr));
    };
    return Thumb_Load(regs[m], n, t, ldrb_op);
}

int Cpu::Thumb_LdrhImm(u32 imm, Reg n, Reg t) {
    auto ldrh_op = [](Memory& _mem, u32 addr) -> std::tuple<u32, int> {
        return std::make_tuple(RotateRight(_mem.ReadMem<u16>(addr), (addr & 0x1) * 8), _mem.AccessTime<u16>(addr));
    };
    return Thumb_Load(imm << 1, n, t, ldrh_op);
}

int Cpu::Thumb_LdrhReg(Reg m, Reg n, Reg t) {
    auto ldrh_op = [](Memory& _mem, u32 addr) -> std::tuple<u32, int> {
        return std::make_tuple(RotateRight(_mem.ReadMem<u16>(addr), (addr & 0x1) * 8), _mem.AccessTime<u16>(addr));
    };
    return Thumb_Load(regs[m], n, t, ldrh_op);
}

int Cpu::Thumb_LdrsbReg(Reg m, Reg n, Reg t) {
    auto ldrsb_op = [](Memory& _mem, u32 addr) -> std::tuple<u32, int> {
        return std::make_tuple(SignExtend(static_cast<u32>(_mem.ReadMem<u8>(addr)), 8), _mem.AccessTime<u8>(addr));
    };
    return Thumb_Load(regs[m], n, t, ldrsb_op);
}

int Cpu::Thumb_LdrshReg(Reg m, Reg n, Reg t) {
    auto ldrsh_op = [](Memory& _mem, u32 addr) -> std::tuple<u32, int> {
        // LDRSH only sign-extends the first byte after an unaligned access.
        int num_source_bits = 16 >> (addr & 0x1);
        return std::make_tuple(SignExtend(RotateRight(_mem.ReadMem<u16>(addr), (addr & 0x1) * 8), num_source_bits),
                               _mem.AccessTime<u16>(addr));
    };
    return Thumb_Load(regs[m], n, t, ldrsh_op);
}

int Cpu::Thumb_Pop(bool p, u32 reg_list) {
    assert(p || Popcount(reg_list) != 0); // Unpredictable

    const std::bitset<8> rlist{reg_list};
    u32 addr = regs[sp];

    // One internal cycle to transfer the last loaded value to the destination register.
    int cycles = 1;

    for (Reg i = 0; i < 8; ++i) {
        if (rlist[i]) {
            // Reads are aligned.
            regs[i] = mem.ReadMem<u32>(addr);
            cycles += mem.AccessTime<u32>(addr);
            addr += 4;
        }
    }

    LoadInternalCycle(1);

    if (p) {
        cycles += Thumb_BranchWritePC(mem.ReadMem<u32>(addr));
        addr += 4;
    }

    regs[sp] = addr;

    return cycles;
}

// Stores
int Cpu::Thumb_Push(bool m, u32 reg_list) {
    assert(m || Popcount(reg_list) != 0); // Unpredictable

    const std::bitset<8> rlist{reg_list};
    regs[sp] -= 4 * rlist.count();
    if (m) {
        regs[sp] -= 4;
    }
    u32 addr = regs[sp];

    int cycles = 0;
    for (Reg i = 0; i < 8; ++i) {
        if (rlist[i]) {
            // Writes are always aligned.
            mem.WriteMem(addr, regs[i]);
            cycles += mem.AccessTime<u32>(addr);
            addr += 4;
        }
    }

    if (m) {
        mem.WriteMem(addr, regs[lr]);
        cycles += mem.AccessTime<u32>(addr);
    }

    return cycles;
}

int Cpu::Thumb_Stm(Reg n, u32 reg_list) {
    assert(Popcount(reg_list) != 0); // Unpredictable

    const std::bitset<8> rlist{reg_list};
    u32 addr = regs[n];

    int cycles = 0;
    for (Reg i = 0; i < 8; ++i) {
        if (rlist[i]) {
            // Writes are always aligned.
            if (i == n && n != LowestSetBit(reg_list)) {
                // Store the new Rn value if it's not the first register in the list.
                mem.WriteMem(addr, static_cast<u32>(regs[n] + 4 * rlist.count()));
            } else {
                mem.WriteMem(addr, regs[i]);
            }
            cycles += mem.AccessTime<u32>(addr);
            addr += 4;
        }
    }

    regs[n] = addr;

    return cycles;
}

int Cpu::Thumb_StrImm(u32 imm, Reg n, Reg t) {
    auto str_op = [](Memory& _mem, u32 addr, u32 data) -> int {
        _mem.WriteMem(addr, data);
        return _mem.AccessTime<u32>(addr);
    };
    return Thumb_Store(imm << 2, n, t, str_op);
}

int Cpu::Thumb_StrSpImm(Reg t, u32 imm) {
    auto str_op = [](Memory& _mem, u32 addr, u32 data) -> int {
        _mem.WriteMem(addr, data);
        return _mem.AccessTime<u32>(addr);
    };
    return Thumb_Store(imm << 2, sp, t, str_op);
}

int Cpu::Thumb_StrReg(Reg m, Reg n, Reg t) {
    auto str_op = [](Memory& _mem, u32 addr, u32 data) -> int {
        _mem.WriteMem(addr, data);
        return _mem.AccessTime<u32>(addr);
    };
    return Thumb_Store(regs[m], n, t, str_op);
}

int Cpu::Thumb_StrbImm(u32 imm, Reg n, Reg t) {
    auto strb_op = [](Memory& _mem, u32 addr, u32 data) -> int {
        _mem.WriteMem<u8>(addr, data);
        return _mem.AccessTime<u8>(addr);
    };
    return Thumb_Store(imm, n, t, strb_op);
}

int Cpu::Thumb_StrbReg(Reg m, Reg n, Reg t) {
    auto strb_op = [](Memory& _mem, u32 addr, u32 data) -> int {
        _mem.WriteMem<u8>(addr, data);
        return _mem.AccessTime<u8>(addr);
    };
    return Thumb_Store(regs[m], n, t, strb_op);
}

int Cpu::Thumb_StrhImm(u32 imm, Reg n, Reg t) {
    auto strh_op = [](Memory& _mem, u32 addr, u32 data) -> int {
        _mem.WriteMem<u16>(addr, data);
        return _mem.AccessTime<u16>(addr);
    };
    return Thumb_Store(imm << 1, n, t, strh_op);
}

int Cpu::Thumb_StrhReg(Reg m, Reg n, Reg t) {
    auto strh_op = [](Memory& _mem, u32 addr, u32 data) -> int {
        _mem.WriteMem<u16>(addr, data);
        return _mem.AccessTime<u16>(addr);
    };
    return Thumb_Store(regs[m], n, t, strh_op);
}

// Misc
int Cpu::Thumb_Swi(u32) {
    return TakeException(CpuMode::Svc);
}

int Cpu::Thumb_Undefined(u16) {
    return TakeException(CpuMode::Undef);
}

} // End namespace Gba
