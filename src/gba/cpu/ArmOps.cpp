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

#include "gba/cpu/Cpu.h"
#include "gba/memory/Memory.h"

namespace Gba {

// Arithmetic Operators
int Cpu::Arm_AdcImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    imm = ArmExpandImmediate(imm);

    u64 result = AddWithCarry(regs[n], imm, GetCarry());

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_AdcReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 shifted_reg = Shift(regs[m], shift.type, shift.imm, GetCarry());
    u64 result = AddWithCarry(regs[n], shifted_reg, GetCarry());

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_AdcRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    assert(d != pc && n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 shifted_reg = Shift(regs[m], type, regs[s] & 0xFF, GetCarry());
    u64 result = AddWithCarry(regs[n], shifted_reg, GetCarry());

    regs[d] = result;
    ConditionalSetAllFlags(set_flags, result);

    return 1;
}

int Cpu::Arm_AddImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    imm = ArmExpandImmediate(imm);

    u64 result = AddWithCarry(regs[n], imm, 0);
    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_AddReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 shifted_reg = Shift(regs[m], shift.type, shift.imm, GetCarry());
    u64 result = AddWithCarry(regs[n], shifted_reg, 0);

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_AddRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    assert(d != pc && n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 shifted_reg = Shift(regs[m], type, regs[s] & 0xFF, GetCarry());
    u64 result = AddWithCarry(regs[n], shifted_reg, 0);

    regs[d] = result;
    ConditionalSetAllFlags(set_flags, result);

    return 1;
}

int Cpu::Arm_CmnImm(Condition cond, Reg n, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    imm = ArmExpandImmediate(imm);

    u64 result = AddWithCarry(regs[n], imm, 0);
    SetAllFlags(result);

    return 1;
}

int Cpu::Arm_CmnReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 shifted_reg = Shift(regs[m], shift.type, shift.imm, GetCarry());
    u64 result = AddWithCarry(regs[n], shifted_reg, 0);

    SetAllFlags(result);

    return 1;
}

int Cpu::Arm_CmnRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m) {
    assert(n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 shifted_reg = Shift(regs[m], type, regs[s] & 0xFF, GetCarry());
    u64 result = AddWithCarry(regs[n], shifted_reg, 0);

    SetAllFlags(result);

    return 1;
}

int Cpu::Arm_CmpImm(Condition cond, Reg n, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    imm = ArmExpandImmediate(imm);

    u64 result = AddWithCarry(regs[n], ~imm, 1);
    SetAllFlags(result);

    return 1;
}

int Cpu::Arm_CmpReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 shifted_reg = Shift(regs[m], shift.type, shift.imm, GetCarry());
    u64 result = AddWithCarry(regs[n], ~shifted_reg, 1);

    SetAllFlags(result);

    return 1;
}

int Cpu::Arm_CmpRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m) {
    assert(n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 shifted_reg = Shift(regs[m], type, regs[s] & 0xFF, GetCarry());
    u64 result = AddWithCarry(regs[n], ~shifted_reg, 1);

    SetAllFlags(result);

    return 1;
}

int Cpu::Arm_MlaReg(Condition cond, bool set_flags, Reg d, Reg a, Reg m, Reg n) {
    assert(d != pc && n != pc && m != pc && a != pc && d != n); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 result = regs[n] * regs[m] + regs[a];

    regs[d] = result;
    // The carry flag gets destroyed on ARMv4.
    ConditionalSetSignZeroCarryFlags(set_flags, result, 0);

    return 1;
}

int Cpu::Arm_MulReg(Condition cond, bool set_flags, Reg d, Reg m, Reg n) {
    assert(d != pc && n != pc && m != pc && d != n); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 result = regs[n] * regs[m];

    regs[d] = result;
    // The carry flag gets destroyed on ARMv4.
    ConditionalSetSignZeroCarryFlags(set_flags, result, 0);

    return 1;
}

int Cpu::Arm_RsbImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    imm = ArmExpandImmediate(imm);

    u64 result = AddWithCarry(~regs[n], imm, 1);
    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_RsbReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 shifted_reg = Shift(regs[m], shift.type, shift.imm, GetCarry());
    u64 result = AddWithCarry(~regs[n], shifted_reg, 1);

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_RsbRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    assert(d != pc && n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 shifted_reg = Shift(regs[m], type, regs[s] & 0xFF, GetCarry());
    u64 result = AddWithCarry(~regs[n], shifted_reg, 1);

    regs[d] = result;
    ConditionalSetAllFlags(set_flags, result);

    return 1;
}

int Cpu::Arm_RscImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    imm = ArmExpandImmediate(imm);

    u64 result = AddWithCarry(~regs[n], imm, GetCarry());

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_RscReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 shifted_reg = Shift(regs[m], shift.type, shift.imm, GetCarry());
    u64 result = AddWithCarry(~regs[n], shifted_reg, GetCarry());

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_RscRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    assert(d != pc && n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 shifted_reg = Shift(regs[m], type, regs[s] & 0xFF, GetCarry());
    u64 result = AddWithCarry(~regs[n], shifted_reg, GetCarry());

    regs[d] = result;
    ConditionalSetAllFlags(set_flags, result);

    return 1;
}

int Cpu::Arm_SbcImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    imm = ArmExpandImmediate(imm);

    u64 result = AddWithCarry(regs[n], ~imm, GetCarry());

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_SbcReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 shifted_reg = Shift(regs[m], shift.type, shift.imm, GetCarry());
    u64 result = AddWithCarry(regs[n], ~shifted_reg, GetCarry());

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_SbcRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    assert(d != pc && n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 shifted_reg = Shift(regs[m], type, regs[s] & 0xFF, GetCarry());
    u64 result = AddWithCarry(regs[n], ~shifted_reg, GetCarry());

    regs[d] = result;
    ConditionalSetAllFlags(set_flags, result);

    return 1;
}

int Cpu::Arm_SmlalReg(Condition cond, bool set_flags, Reg dh, Reg dl, Reg m, Reg n) {
    assert(dh != pc && dl != pc && m != pc && n != pc && dh != n && dl != n && dh != dl); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    s64 result = static_cast<s64>(static_cast<s32>(regs[n])) * static_cast<s64>(static_cast<s32>(regs[m]));
    result += static_cast<s64>((static_cast<u64>(regs[dh]) << 32) | regs[dl]);

    regs[dh] = result >> 32;
    regs[dl] = result;
    // The carry and overflow flags get destroyed on ARMv4.
    ConditionalSetMultiplyLongFlags(set_flags, result);

    return 1;
}

int Cpu::Arm_SmullReg(Condition cond, bool set_flags, Reg dh, Reg dl, Reg m, Reg n) {
    assert(dh != pc && dl != pc && m != pc && n != pc && dh != n && dl != n && dh != dl); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    s64 result = static_cast<s64>(static_cast<s32>(regs[n])) * static_cast<s64>(static_cast<s32>(regs[m]));

    regs[dh] = result >> 32;
    regs[dl] = result;
    // The carry and overflow flags get destroyed on ARMv4.
    ConditionalSetMultiplyLongFlags(set_flags, result);

    return 1;
}

int Cpu::Arm_SubImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    imm = ArmExpandImmediate(imm);

    u64 result = AddWithCarry(regs[n], ~imm, 1);

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_SubReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 shifted_reg = Shift(regs[m], shift.type, shift.imm, GetCarry());
    u64 result = AddWithCarry(regs[n], ~shifted_reg, 1);

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetAllFlags(set_flags, result);
    }

    return 1;
}

int Cpu::Arm_SubRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    assert(d != pc && n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 shifted_reg = Shift(regs[m], type, regs[s] & 0xFF, GetCarry());
    u64 result = AddWithCarry(regs[n], ~shifted_reg, 1);

    regs[d] = result;
    ConditionalSetAllFlags(set_flags, result);

    return 1;
}

int Cpu::Arm_UmlalReg(Condition cond, bool set_flags, Reg dh, Reg dl, Reg m, Reg n) {
    assert(dh != pc && dl != pc && m != pc && n != pc && dh != n && dl != n && dh != dl); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u64 result = static_cast<u64>(regs[n]) * static_cast<u64>(regs[m]);
    result += (static_cast<u64>(regs[dh]) << 32) | regs[dl];

    regs[dh] = result >> 32;
    regs[dl] = result;
    // The carry and overflow flags get destroyed on ARMv4.
    ConditionalSetMultiplyLongFlags(set_flags, result);

    return 1;
}

int Cpu::Arm_UmullReg(Condition cond, bool set_flags, Reg dh, Reg dl, Reg m, Reg n) {
    assert(dh != pc && dl != pc && m != pc && n != pc && dh != n && dl != n && dh != dl); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u64 result = static_cast<u64>(regs[n]) * static_cast<u64>(regs[m]);

    regs[dh] = result >> 32;
    regs[dl] = result;
    // The carry and overflow flags get destroyed on ARMv4.
    ConditionalSetMultiplyLongFlags(set_flags, result);

    return 1;
}

// Logical Operators
int Cpu::Arm_AndImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry expanded_imm = ArmExpandImmediate_C(imm);

    u32 result = regs[n] & expanded_imm.result;
    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetSignZeroCarryFlags(set_flags, result, expanded_imm.carry);
    }

    return 1;
}

int Cpu::Arm_AndReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], shift.type, shift.imm, GetCarry());
    u32 result = regs[n] & shifted_reg.result;

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetSignZeroCarryFlags(set_flags, result, shifted_reg.carry);
    }

    return 1;
}

int Cpu::Arm_AndRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    assert(d != pc && n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry shifted_reg = Shift_C(regs[m], type, regs[s] & 0xFF, GetCarry());
    u32 result = regs[n] & shifted_reg.result;

    regs[d] = result;
    ConditionalSetSignZeroCarryFlags(set_flags, result, shifted_reg.carry);

    return 1;
}

int Cpu::Arm_BicImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry expanded_imm = ArmExpandImmediate_C(imm);

    u32 result = regs[n] & ~expanded_imm.result;
    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetSignZeroCarryFlags(set_flags, result, expanded_imm.carry);
    }

    return 1;
}

int Cpu::Arm_BicReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], shift.type, shift.imm, GetCarry());
    u32 result = regs[n] & ~shifted_reg.result;

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetSignZeroCarryFlags(set_flags, result, shifted_reg.carry);
    }

    return 1;
}

int Cpu::Arm_BicRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    assert(d != pc && n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry shifted_reg = Shift_C(regs[m], type, regs[s] & 0xFF, GetCarry());
    u32 result = regs[n] & ~shifted_reg.result;

    regs[d] = result;
    ConditionalSetSignZeroCarryFlags(set_flags, result, shifted_reg.carry);

    return 1;
}

int Cpu::Arm_EorImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry expanded_imm = ArmExpandImmediate_C(imm);

    u32 result = regs[n] ^ expanded_imm.result;
    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetSignZeroCarryFlags(set_flags, result, expanded_imm.carry);
    }

    return 1;
}

int Cpu::Arm_EorReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], shift.type, shift.imm, GetCarry());
    u32 result = regs[n] ^ shifted_reg.result;

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetSignZeroCarryFlags(set_flags, result, shifted_reg.carry);
    }

    return 1;
}

int Cpu::Arm_EorRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    assert(d != pc && n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry shifted_reg = Shift_C(regs[m], type, regs[s] & 0xFF, GetCarry());
    u32 result = regs[n] ^ shifted_reg.result;

    regs[d] = result;
    ConditionalSetSignZeroCarryFlags(set_flags, result, shifted_reg.carry);

    return 1;
}

int Cpu::Arm_OrrImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry expanded_imm = ArmExpandImmediate_C(imm);

    u32 result = regs[n] | expanded_imm.result;
    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetSignZeroCarryFlags(set_flags, result, expanded_imm.carry);
    }

    return 1;
}

int Cpu::Arm_OrrReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], shift.type, shift.imm, GetCarry());
    u32 result = regs[n] | shifted_reg.result;

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(result);
        } else {
            Arm_BranchWritePC(result);
        }
    } else {
        regs[d] = result;
        ConditionalSetSignZeroCarryFlags(set_flags, result, shifted_reg.carry);
    }

    return 1;
}

int Cpu::Arm_OrrRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    assert(d != pc && n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry shifted_reg = Shift_C(regs[m], type, regs[s] & 0xFF, GetCarry());
    u32 result = regs[n] | shifted_reg.result;

    regs[d] = result;
    ConditionalSetSignZeroCarryFlags(set_flags, result, shifted_reg.carry);

    return 1;
}

int Cpu::Arm_TeqImm(Condition cond, Reg n, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry expanded_imm = ArmExpandImmediate_C(imm);

    u32 result = regs[n] ^ expanded_imm.result;
    SetSignZeroCarryFlags(result, expanded_imm.carry);

    return 1;
}

int Cpu::Arm_TeqReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], shift.type, shift.imm, GetCarry());
    u32 result = regs[n] ^ shifted_reg.result;

    SetSignZeroCarryFlags(result, shifted_reg.carry);

    return 1;
}

int Cpu::Arm_TeqRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m) {
    assert(n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry shifted_reg = Shift_C(regs[m], type, regs[s] & 0xFF, GetCarry());
    u32 result = regs[n] ^ shifted_reg.result;

    SetSignZeroCarryFlags(result, shifted_reg.carry);

    return 1;
}

int Cpu::Arm_TstImm(Condition cond, Reg n, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry expanded_imm = ArmExpandImmediate_C(imm);

    u32 result = regs[n] & expanded_imm.result;
    SetSignZeroCarryFlags(result, expanded_imm.carry);

    return 1;
}

int Cpu::Arm_TstReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], shift.type, shift.imm, GetCarry());
    u32 result = regs[n] & shifted_reg.result;

    SetSignZeroCarryFlags(result, shifted_reg.carry);

    return 1;
}

int Cpu::Arm_TstRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m) {
    assert(n != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry shifted_reg = Shift_C(regs[m], type, regs[s] & 0xFF, GetCarry());
    u32 result = regs[n] & shifted_reg.result;

    SetSignZeroCarryFlags(result, shifted_reg.carry);

    return 1;
}

// Shifts
int Cpu::Arm_AsrImm(Condition cond, bool set_flags, Reg d, u32 imm, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(ShiftType::ASR, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], ShiftType::ASR, shift.imm, GetCarry());

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(shifted_reg.result);
        } else {
            Arm_BranchWritePC(shifted_reg.result);
        }
    } else {
        regs[d] = shifted_reg.result;
        ConditionalSetSignZeroCarryFlags(set_flags, shifted_reg.result, shifted_reg.carry);
    }

    return 1;
}

int Cpu::Arm_AsrReg(Condition cond, bool set_flags, Reg d, Reg m, Reg n) {
    assert(d != pc && n != pc && m != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry shifted_reg = Shift_C(regs[n], ShiftType::ASR, regs[m] & 0xFF, GetCarry());

    regs[d] = shifted_reg.result;
    ConditionalSetSignZeroCarryFlags(set_flags, shifted_reg.result, shifted_reg.carry);

    return 1;
}

int Cpu::Arm_LslImm(Condition cond, bool set_flags, Reg d, u32 imm, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    // No need to DecodeImmShift for LSL.
    ResultWithCarry shifted_reg = Shift_C(regs[m], ShiftType::LSL, imm, GetCarry());

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(shifted_reg.result);
        } else {
            Arm_BranchWritePC(shifted_reg.result);
        }
    } else {
        regs[d] = shifted_reg.result;
        ConditionalSetSignZeroCarryFlags(set_flags, shifted_reg.result, shifted_reg.carry);
    }

    return 1;
}

int Cpu::Arm_LslReg(Condition cond, bool set_flags, Reg d, Reg m, Reg n) {
    assert(d != pc && n != pc && m != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry shifted_reg = Shift_C(regs[n], ShiftType::LSL, regs[m] & 0xFF, GetCarry());

    regs[d] = shifted_reg.result;
    ConditionalSetSignZeroCarryFlags(set_flags, shifted_reg.result, shifted_reg.carry);

    return 1;
}

int Cpu::Arm_LsrImm(Condition cond, bool set_flags, Reg d, u32 imm, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(ShiftType::LSR, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], ShiftType::LSR, shift.imm, GetCarry());

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(shifted_reg.result);
        } else {
            Arm_BranchWritePC(shifted_reg.result);
        }
    } else {
        regs[d] = shifted_reg.result;
        ConditionalSetSignZeroCarryFlags(set_flags, shifted_reg.result, shifted_reg.carry);
    }

    return 1;
}

int Cpu::Arm_LsrReg(Condition cond, bool set_flags, Reg d, Reg m, Reg n) {
    assert(d != pc && n != pc && m != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry shifted_reg = Shift_C(regs[n], ShiftType::LSR, regs[m] & 0xFF, GetCarry());

    regs[d] = shifted_reg.result;
    ConditionalSetSignZeroCarryFlags(set_flags, shifted_reg.result, shifted_reg.carry);

    return 1;
}

int Cpu::Arm_RorImm(Condition cond, bool set_flags, Reg d, u32 imm, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    // RRX if imm is 0.
    ImmediateShift shift = DecodeImmShift(ShiftType::ROR, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], shift.type, shift.imm, GetCarry());

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(shifted_reg.result);
        } else {
            Arm_BranchWritePC(shifted_reg.result);
        }
    } else {
        regs[d] = shifted_reg.result;
        ConditionalSetSignZeroCarryFlags(set_flags, shifted_reg.result, shifted_reg.carry);
    }

    return 1;
}

int Cpu::Arm_RorReg(Condition cond, bool set_flags, Reg d, Reg m, Reg n) {
    assert(d != pc && n != pc && m != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry shifted_reg = Shift_C(regs[n], ShiftType::ROR, regs[m] & 0xFF, GetCarry());

    regs[d] = shifted_reg.result;
    ConditionalSetSignZeroCarryFlags(set_flags, shifted_reg.result, shifted_reg.carry);

    return 1;
}

// Branches
int Cpu::Arm_B(Condition cond, u32 imm24) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    s32 signed_imm32 = SignExtend(imm24 << 2, 26);

    Arm_BranchWritePC(regs[pc] + signed_imm32);

    return 1;
}

int Cpu::Arm_Bl(Condition cond, u32 imm24) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    s32 signed_imm32 = SignExtend(imm24 << 2, 26);

    regs[lr] = regs[pc] - 4;
    Arm_BranchWritePC(regs[pc] + signed_imm32);

    return 1;
}

int Cpu::Arm_Bx(Condition cond, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    BxWritePC(regs[m]);

    return 1;
}

// Moves
int Cpu::Arm_MovImm(Condition cond, bool set_flags, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry expanded_imm = ArmExpandImmediate_C(imm);

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(expanded_imm.result);
        } else {
            Arm_BranchWritePC(expanded_imm.result);
        }
    } else {
        regs[d] = expanded_imm.result;
        ConditionalSetSignZeroCarryFlags(set_flags, expanded_imm.result, expanded_imm.carry);
    }

    return 1;
}

int Cpu::Arm_MovReg(Condition cond, bool set_flags, Reg d, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(regs[m]);
        } else {
            Arm_BranchWritePC(regs[m]);
        }
    } else {
        regs[d] = regs[m];
        ConditionalSetSignZeroFlags(set_flags, regs[m]);
    }

    return 1;
}

int Cpu::Arm_MvnImm(Condition cond, bool set_flags, Reg d, u32 imm) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry expanded_imm = ArmExpandImmediate_C(imm);
    expanded_imm.result = ~expanded_imm.result;

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(expanded_imm.result);
        } else {
            Arm_BranchWritePC(expanded_imm.result);
        }
    } else {
        regs[d] = expanded_imm.result;
        ConditionalSetSignZeroCarryFlags(set_flags, expanded_imm.result, expanded_imm.carry);
    }

    return 1;
}

int Cpu::Arm_MvnReg(Condition cond, bool set_flags, Reg d, u32 imm, ShiftType type, Reg m) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    ResultWithCarry shifted_reg = Shift_C(regs[m], shift.type, shift.imm, GetCarry());
    shifted_reg.result = ~shifted_reg.result;

    if (d == pc) {
        if (set_flags && HasSpsr()) {
            ReturnFromException(shifted_reg.result);
        } else {
            Arm_BranchWritePC(shifted_reg.result);
        }
    } else {
        regs[d] = shifted_reg.result;
        ConditionalSetSignZeroCarryFlags(set_flags, shifted_reg.result, shifted_reg.carry);
    }

    return 1;
}

int Cpu::Arm_MvnRegShifted(Condition cond, bool set_flags, Reg d, Reg s, ShiftType type, Reg m) {
    assert(d != pc && m != pc && s != pc); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ResultWithCarry shifted_reg = Shift_C(regs[m], type, regs[s] & 0xFF, GetCarry());
    shifted_reg.result = ~shifted_reg.result;

    regs[d] = shifted_reg.result;
    ConditionalSetSignZeroCarryFlags(set_flags, shifted_reg.result, shifted_reg.carry);

    return 1;
}

// Loads
int Cpu::Arm_Ldmi(Condition cond, bool pre_indexed, bool exception_return, bool writeback, Reg n, u32 reg_list) {
    assert(n != pc && Popcount(reg_list) != 0); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    const std::bitset<16> rlist{reg_list};
    u32 addr = regs[n];
    if (pre_indexed) {
        addr += 4;
    }

    // Loading user regs is unpredictable in User and System modes. I assume it just does a normal LDM in that case.
    bool load_user_regs = exception_return && !rlist[pc] && HasSpsr();
    CpuMode current_cpu_mode;
    if (load_user_regs) {
        current_cpu_mode = CurrentCpuMode();
        CpuModeSwitch(CpuMode::User);
    }

    for (Reg i = 0; i < 15; ++i) {
        if (rlist[i]) {
            // Reads must be aligned.
            regs[i] = mem.ReadMem<u32>(addr & ~0x3);
            addr += 4;
        }
    }

    if (load_user_regs) {
        CpuModeSwitch(current_cpu_mode);
    }

    // Only write back to Rn if it wasn't in the register list (ARM7TDMI behaviour).
    if (writeback && !rlist[n]) {
        regs[n] += 4 * rlist.count();
    }

    if (rlist[pc]) {
        if (exception_return && HasSpsr()) {
            ReturnFromException(mem.ReadMem<u32>(addr & ~0x3));
        } else {
            Arm_BranchWritePC(mem.ReadMem<u32>(addr & ~0x3));
        }
    }

    return 1;
}

int Cpu::Arm_Ldmd(Condition cond, bool pre_indexed, bool exception_return, bool writeback, Reg n, u32 reg_list) {
    assert(n != pc && Popcount(reg_list) != 0); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    const std::bitset<16> rlist{reg_list};
    u32 addr = regs[n] - 4 * rlist.count();
    if (!pre_indexed) {
        addr += 4;
    }

    // Loading user regs is unpredictable in User and System modes. I assume it just does a normal LDM in that case.
    bool load_user_regs = exception_return && !rlist[pc] && HasSpsr();
    CpuMode current_cpu_mode;
    if (load_user_regs) {
        current_cpu_mode = CurrentCpuMode();
        CpuModeSwitch(CpuMode::User);
    }

    for (Reg i = 0; i < 15; ++i) {
        if (rlist[i]) {
            // Reads must be aligned.
            regs[i] = mem.ReadMem<u32>(addr & ~0x3);
            addr += 4;
        }
    }

    if (load_user_regs) {
        CpuModeSwitch(current_cpu_mode);
    }

    // Only write back to Rn if it wasn't in the register list (ARM7TDMI behaviour).
    if (writeback && !rlist[n]) {
        regs[n] -= 4 * rlist.count();
    }

    if (rlist[pc]) {
        if (exception_return && HasSpsr()) {
            ReturnFromException(mem.ReadMem<u32>(addr & ~0x3));
        } else {
            Arm_BranchWritePC(mem.ReadMem<u32>(addr & ~0x3));
        }
    }

    return 1;
}

int Cpu::Arm_LdrImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm) {
    writeback = writeback || !pre_indexed;
    assert(!(writeback && n == t)); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    if (!add) {
        imm = -imm;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += imm;
    }

    u32 data = mem.ReadMem<u32>(addr);

    if (writeback) {
        regs[n] += imm;
    }

    if (t == pc) {
        assert((addr & 0x3) == 0x0); // Unpredictable
        Arm_BranchWritePC(data);
    } else {
        regs[t] = data;
    }

    return 1;
}

int Cpu::Arm_LdrReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm,
                    ShiftType type, Reg m) {
    writeback = writeback || !pre_indexed;
    assert(m != pc && !(writeback && (n == pc || n == t || n == m))); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 offset = Shift(regs[m], shift.type, shift.imm, GetCarry());

    if (!add) {
        offset = -offset;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += offset;
    }

    u32 data = mem.ReadMem<u32>(addr);

    if (writeback) {
        regs[n] += offset;
    }

    if (t == pc) {
        assert((addr & 0x3) == 0x0); // Unpredictable
        Arm_BranchWritePC(data);
    } else {
        regs[t] = data;
    }

    return 1;
}

int Cpu::Arm_LdrbImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm) {
    writeback = writeback || !pre_indexed;
    assert(t != pc && !(writeback && n == t)); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    if (!add) {
        imm = -imm;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += imm;
    }

    regs[t] = mem.ReadMem<u8>(addr);

    if (writeback) {
        regs[n] += imm;
    }

    return 1;
}

int Cpu::Arm_LdrbReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm,
                    ShiftType type, Reg m) {
    writeback = writeback || !pre_indexed;
    assert(m != pc && t != pc && !(writeback && (n == pc || n == t || n == m))); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 offset = Shift(regs[m], shift.type, shift.imm, GetCarry());

    if (!add) {
        offset = -offset;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += offset;
    }

    regs[t] = mem.ReadMem<u8>(addr);

    if (writeback) {
        regs[n] += offset;
    }

    return 1;
}

int Cpu::Arm_LdrhImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm_hi,
                     u32 imm_lo) {
    writeback = writeback || !pre_indexed;
    assert(t != pc && !(writeback && n == t)); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 imm = (imm_hi << 4) | imm_lo;

    if (!add) {
        imm = -imm;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += imm;
    }

    regs[t] = mem.ReadMem<u16>(addr);

    if (writeback) {
        regs[n] += imm;
    }

    return 1;
}

int Cpu::Arm_LdrhReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, Reg m) {
    writeback = writeback || !pre_indexed;
    assert(m != pc && t != pc && !(writeback && (n == pc || n == t || n == m))); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 offset = regs[m];

    if (!add) {
        offset = -offset;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += offset;
    }

    regs[t] = mem.ReadMem<u16>(addr);

    if (writeback) {
        regs[n] += offset;
    }

    return 1;
}

int Cpu::Arm_LdrsbImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm_hi,
                     u32 imm_lo) {
    writeback = writeback || !pre_indexed;
    assert(t != pc && !(writeback && n == t)); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 imm = (imm_hi << 4) | imm_lo;

    if (!add) {
        imm = -imm;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += imm;
    }

    regs[t] = SignExtend(static_cast<u32>(mem.ReadMem<u8>(addr)), 8);

    if (writeback) {
        regs[n] += imm;
    }

    return 1;
}

int Cpu::Arm_LdrsbReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, Reg m) {
    writeback = writeback || !pre_indexed;
    assert(m != pc && t != pc && !(writeback && (n == pc || n == t || n == m))); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 offset = regs[m];

    if (!add) {
        offset = -offset;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += offset;
    }

    regs[t] = SignExtend(static_cast<u32>(mem.ReadMem<u8>(addr)), 8);

    if (writeback) {
        regs[n] += offset;
    }

    return 1;
}

int Cpu::Arm_LdrshImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm_hi,
                     u32 imm_lo) {
    writeback = writeback || !pre_indexed;
    assert(t != pc && !(writeback && n == t)); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 imm = (imm_hi << 4) | imm_lo;

    if (!add) {
        imm = -imm;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += imm;
    }

    regs[t] = SignExtend(static_cast<u32>(mem.ReadMem<u16>(addr)), 16);

    if (writeback) {
        regs[n] += imm;
    }

    return 1;
}

int Cpu::Arm_LdrshReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, Reg m) {
    writeback = writeback || !pre_indexed;
    assert(m != pc && t != pc && !(writeback && (n == pc || n == t || n == m))); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 offset = regs[m];

    if (!add) {
        offset = -offset;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += offset;
    }

    regs[t] = SignExtend(static_cast<u32>(mem.ReadMem<u16>(addr)), 16);

    if (writeback) {
        regs[n] += offset;
    }

    return 1;
}

int Cpu::Arm_PopA1(Condition cond, u32 reg_list) {
    assert(Popcount(reg_list) != 0); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    const std::bitset<16> rlist{reg_list};
    u32 addr = regs[sp];

    for (Reg i = 0; i < 15; ++i) {
        if (rlist[i]) {
            // Reads must be aligned.
            regs[i] = mem.ReadMem<u32>(addr & ~0x3);
            addr += 4;
        }
    }

    if (rlist[pc]) {
        Arm_BranchWritePC(mem.ReadMem<u32>(addr & ~0x3));
    }

    // Only write back to SP if it wasn't in the register list (ARM7TDMI behaviour).
    if (!rlist[sp]) {
        regs[sp] += 4 * rlist.count();
    }

    return 1;
}

int Cpu::Arm_PopA2(Condition cond, Reg t) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    if (t == pc) {
        Arm_BranchWritePC(mem.ReadMem<u32>(regs[sp]));
    } else {
        // Unaligned reads are allowed.
        regs[t] = mem.ReadMem<u32>(regs[sp]);
    }

    // Only write back to SP if it wasn't in the register list (ARM7TDMI behaviour).
    if (t != sp) {
        regs[sp] += 4;
    }

    return 1;
}

// Stores
int Cpu::Arm_PushA1(Condition cond, u32 reg_list) {
    assert(Popcount(reg_list) != 0); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    const std::bitset<16> rlist{reg_list};
    u32 addr = regs[sp] - 4 * rlist.count();

    for (Reg i = 0; i < 16; ++i) {
        if (rlist[i]) {
            // Writes are always aligned.
            if (i == sp && sp != LowestSetBit(reg_list)) {
                // Store the new SP value if it's not the first register in the list.
                mem.WriteMem(addr, static_cast<u32>(regs[sp] - 4 * rlist.count()));
            } else {
                mem.WriteMem(addr, regs[i]);
            }
            addr += 4;
        }
    }

    // Only write back to SP if it wasn't in the register list (ARM7TDMI behaviour).
    if (!rlist[sp]) {
        regs[sp] -= 4 * rlist.count();
    }

    return 1;
}

int Cpu::Arm_PushA2(Condition cond, Reg t) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    mem.WriteMem(regs[sp], regs[t]);

    // Only write back to SP if it wasn't in the register list (ARM7TDMI behaviour).
    if (t != sp) {
        regs[sp] += 4;
    }

    return 1;
}

int Cpu::Arm_Stmi(Condition cond, bool pre_indexed, bool store_user_regs, bool writeback, Reg n, u32 reg_list) {
    assert(n != pc && Popcount(reg_list) != 0); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    const std::bitset<16> rlist{reg_list};
    u32 addr = regs[n];
    if (pre_indexed) {
        addr += 4;
    }

    // Storing user regs is unpredictable in User and System modes. I assume it just does a normal STM in that case.
    store_user_regs = store_user_regs && HasSpsr();
    CpuMode current_cpu_mode;
    if (store_user_regs) {
        current_cpu_mode = CurrentCpuMode();
        CpuModeSwitch(CpuMode::User);
    }

    for (Reg i = 0; i < 16; ++i) {
        if (rlist[i]) {
            // Writes are always aligned.
            if (i == n && writeback && n != LowestSetBit(reg_list)) {
                // Store the new Rn value if it's not the first register in the list.
                mem.WriteMem(addr, static_cast<u32>(regs[n] + 4 * rlist.count()));
            } else {
                mem.WriteMem(addr, regs[i]);
            }
            addr += 4;
        }
    }

    if (store_user_regs) {
        CpuModeSwitch(current_cpu_mode);
    }

    if (writeback) {
        regs[n] += 4 * rlist.count();
    }

    return 1;
}

int Cpu::Arm_Stmd(Condition cond, bool pre_indexed, bool store_user_regs, bool writeback, Reg n, u32 reg_list) {
    assert(n != pc && Popcount(reg_list) != 0); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    const std::bitset<16> rlist{reg_list};
    u32 addr = regs[n] - 4 * rlist.count();
    if (!pre_indexed) {
        addr += 4;
    }

    // Storing user regs is unpredictable in User and System modes. I assume it just does a normal STM in that case.
    store_user_regs = store_user_regs && HasSpsr();
    CpuMode current_cpu_mode;
    if (store_user_regs) {
        current_cpu_mode = CurrentCpuMode();
        CpuModeSwitch(CpuMode::User);
    }

    for (Reg i = 0; i < 16; ++i) {
        if (rlist[i]) {
            // Writes are always aligned.
            if (i == n && writeback && n != LowestSetBit(reg_list)) {
                // Store the new Rn value if it's not the first register in the list.
                mem.WriteMem(addr, static_cast<u32>(regs[n] - 4 * rlist.count()));
            } else {
                mem.WriteMem(addr, regs[i]);
            }
            addr += 4;
        }
    }

    if (store_user_regs) {
        CpuModeSwitch(current_cpu_mode);
    }

    if (writeback) {
        regs[n] -= 4 * rlist.count();
    }

    return 1;
}

int Cpu::Arm_StrImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm) {
    writeback = writeback || !pre_indexed;
    assert(!(writeback && (n == t || n == pc))); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    if (!add) {
        imm = -imm;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += imm;
    }

    mem.WriteMem(addr, regs[t]);

    if (writeback) {
        regs[n] += imm;
    }

    return 1;
}

int Cpu::Arm_StrReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm,
                    ShiftType type, Reg m) {
    writeback = writeback || !pre_indexed;
    assert(m != pc && !(writeback && (n == pc || n == t || n == m))); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 offset = Shift(regs[m], shift.type, shift.imm, GetCarry());

    if (!add) {
        offset = -offset;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += offset;
    }

    mem.WriteMem(addr, regs[t]);

    if (writeback) {
        regs[n] += offset;
    }

    return 1;
}

int Cpu::Arm_StrbImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm) {
    writeback = writeback || !pre_indexed;
    assert(t != pc && !(writeback && (n == t || n == pc))); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    if (!add) {
        imm = -imm;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += imm;
    }

    mem.WriteMem<u8>(addr, regs[t]);

    if (writeback) {
        regs[n] += imm;
    }

    return 1;
}

int Cpu::Arm_StrbReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm,
                    ShiftType type, Reg m) {
    writeback = writeback || !pre_indexed;
    assert(m != pc && t != pc && !(writeback && (n == pc || n == t || n == m))); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    ImmediateShift shift = DecodeImmShift(type, imm);

    u32 offset = Shift(regs[m], shift.type, shift.imm, GetCarry());

    if (!add) {
        offset = -offset;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += offset;
    }

    mem.WriteMem<u8>(addr, regs[t]);

    if (writeback) {
        regs[n] += offset;
    }

    return 1;
}

int Cpu::Arm_StrhImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm_hi,
                     u32 imm_lo) {
    writeback = writeback || !pre_indexed;
    assert(t != pc && !(writeback && (n == t || n == pc))); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 imm = (imm_hi << 4) | imm_lo;

    if (!add) {
        imm = -imm;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += imm;
    }

    mem.WriteMem<u16>(addr, regs[t]);

    if (writeback) {
        regs[n] += imm;
    }

    return 1;
}

int Cpu::Arm_StrhReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, Reg m) {
    writeback = writeback || !pre_indexed;
    assert(m != pc && t != pc && !(writeback && (n == pc || n == t || n == m))); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 offset = regs[m];

    if (!add) {
        offset = -offset;
    }

    u32 addr = regs[n];
    if (pre_indexed) {
        addr += offset;
    }

    mem.WriteMem<u16>(addr, regs[t]);

    if (writeback) {
        regs[n] += offset;
    }

    return 1;
}

int Cpu::Arm_SwpReg(Condition cond, bool byte, Reg n, Reg t, Reg t2) {
    assert(t != pc && t2 != pc && n == pc || n == t || n == t2); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    u32 data;
    if (byte) {
        data = mem.ReadMem<u8>(regs[n]);
        mem.WriteMem<u8>(regs[n], regs[t2]);
    } else {
        data = mem.ReadMem<u32>(regs[n]);
        mem.WriteMem<u32>(regs[n], regs[t2]);
    }

    regs[t] = data;

    return 1;
}

// Misc
int Cpu::Arm_Cdp(Condition cond, u32, Reg, Reg, u32 coproc, u32, Reg) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    // Access to any coprocessor besides CP14 generates an undefined instruction exception.
    if (coproc != 14) {
        TakeException(CpuMode::Undef);
    }

    return 1;
}

int Cpu::Arm_Ldc(Condition cond, bool p, bool u, bool d, bool w, Reg, Reg, u32 coproc, u32) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    // Access to any coprocessor besides CP14 generates an undefined instruction exception.
    if (coproc != 14 || (!p && !u && !d && !w)) {
        TakeException(CpuMode::Undef);
    }

    return 1;
}

int Cpu::Arm_Mcr(Condition cond, u32, Reg, Reg, u32 coproc, u32, Reg) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    // Access to any coprocessor besides CP14 generates an undefined instruction exception.
    if (coproc != 14) {
        TakeException(CpuMode::Undef);
    }

    return 1;
}

int Cpu::Arm_Mrs(Condition cond, bool read_spsr, Reg d) {
    assert(d != pc); // Unpredictable
    assert(!read_spsr || HasSpsr()); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    if (read_spsr) {
        regs[d] = spsr[CurrentCpuModeIndex()];
    } else {
        // The CPSR is read with the thumb bit masked out.
        regs[d] = cpsr & ~thumb_mode;
    }

    return 1;
}

int Cpu::Arm_MsrImm(Condition cond, bool write_spsr, u32 mask, u32 imm) {
    if (CurrentCpuMode() == CpuMode::User) {
        // Cannot write the control byte in user mode.
        mask &= ~0x1;
    }

    const bool write_control_field = mask & 0x1;

    assert(mask != 0x0); // Unpredictable
    assert(!write_spsr || HasSpsr()); // Unpredictable

    if (!ConditionPassed(cond)) {
        return 1;
    }

    imm = ArmExpandImmediate(imm);

    assert(!write_control_field || ValidCpuMode(imm)); // Probably hangs the CPU.

    // The 4 bits of the "mask" field specify which bytes of the PSR to write.
    // On the ARM7TDMI, only the first and last bytes of the PSRs are used, so we only check the first and last bits.
    // We shift bit 4 by 25 instead of 21 so the last byte mask becomes 0xF0 instead of 0xFF.
    u32 psr_mask = ((mask & 0x1) | ((mask & 0x8) << 25)) * 0xFF;

    if (write_spsr) {
        spsr[CurrentCpuModeIndex()] = imm & psr_mask;
    } else {
        if (write_control_field) {
            CpuModeSwitch(static_cast<CpuMode>(imm & cpu_mode));
        }

        // The thumb bit is masked out when writing the CPSR.
        cpsr = imm & psr_mask & ~thumb_mode;
    }

    return 1;
}

int Cpu::Arm_MsrReg(Condition cond, bool write_spsr, u32 mask, Reg n) {
    if (CurrentCpuMode() == CpuMode::User) {
        // Cannot write the control byte in user mode.
        mask &= ~0x1;
    }

    const bool write_control_field = mask & 0x1;

    assert(mask != 0x0); // Unpredictable
    assert(!write_spsr || HasSpsr()); // Unpredictable
    assert(!write_control_field || ValidCpuMode(regs[n])); // Probably hangs the CPU.

    if (!ConditionPassed(cond)) {
        return 1;
    }

    // The 4 bits of the "mask" field specify which bytes of the PSR to write.
    // On the ARM7TDMI, only the first and last bytes of the PSRs are used, so we only check the first and last bits.
    // We shift bit 4 by 25 instead of 21 so the last byte mask becomes 0xF0 instead of 0xFF.
    u32 psr_mask = ((mask & 0x1) | ((mask & 0x8) << 25)) * 0xFF;

    if (write_spsr) {
        spsr[CurrentCpuModeIndex()] = regs[n] & psr_mask;
    } else {
        if (write_control_field) {
            CpuModeSwitch(static_cast<CpuMode>(regs[n] & cpu_mode));
        }

        // The thumb bit is masked out when writing the CPSR.
        cpsr = regs[n] & psr_mask & ~thumb_mode;
    }

    return 1;
}

int Cpu::Arm_Swi(Condition cond, u32) {
    if (!ConditionPassed(cond)) {
        return 1;
    }

    TakeException(CpuMode::Svc);

    return 1;
}

int Cpu::Arm_Undefined(u32) {
    TakeException(CpuMode::Undef);

    return 1;
}

} // End namespace Gba
