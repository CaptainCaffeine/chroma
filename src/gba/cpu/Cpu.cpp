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
#include "gba/cpu/Instruction.h"
#include "gba/memory/Memory.h"

namespace Gba {

Cpu::Cpu(Memory& memory)
        : mem(memory)
        , thumb_instructions(Instruction<Thumb>::GetInstructionTable())
        , arm_instructions(Instruction<Arm>::GetInstructionTable()) {

    regs[pc] = 0x80000B8;
}

// Needed to declare std::vector with forward-declared type in the header file.
Cpu::~Cpu() = default;

void Cpu::Execute(int cycles) {
    while (cycles > 0) {
        if (ThumbMode()) {
            Thumb opcode = mem.ReadMem<Thumb>(regs[pc]);
            regs[pc] += 2;

            auto impl = DecodeThumb(opcode);

            cycles -= impl(*this, opcode);
        } else {
            Arm opcode = mem.ReadMem<Arm>(regs[pc]);
            regs[pc] += 4;

            auto impl = DecodeArm(opcode);

            cycles -= impl(*this, opcode);
        }
    }
}

std::function<int(Cpu& cpu, Thumb opcode)> Cpu::DecodeThumb(Thumb opcode) const {
    for (const auto& instr : thumb_instructions) {
        if (instr.Match(opcode)) {
            return instr.impl_func;
        }
    }

    return thumb_instructions.back().impl_func;
}

std::function<int(Cpu& cpu, Arm opcode)> Cpu::DecodeArm(Arm opcode) const {
    for (const auto& instr : arm_instructions) {
        if (instr.Match(opcode)) {
            return instr.impl_func;
        }
    }

    return arm_instructions.back().impl_func;
}

Cpu::ImmediateShift Cpu::DecodeImmShift(ShiftType type, u32 imm5) {
    if (imm5 == 0) {
        switch (type) {
        case ShiftType::LSL:
            return {type, imm5};
        case ShiftType::LSR:
            return {type, 32};
        case ShiftType::ASR:
            return {type, 32};
        case ShiftType::ROR:
            return {ShiftType::RRX, 1};
        default:
            assert(false);
            return {ShiftType::LSL, 0};
        }
    }

    return {type, imm5};
}

u32 Cpu::Shift(u32 value, ShiftType type, int shift_amount, u32 carry) {
    assert(!(type == ShiftType::RRX && shift_amount != 1));

    if (shift_amount == 0) {
        return value;
    }

    switch (type) {
    case ShiftType::LSL:
        return value << shift_amount;
    case ShiftType::LSR:
        return value >> shift_amount;
    case ShiftType::ASR:
        return static_cast<s32>(value) >> shift_amount;
    case ShiftType::ROR:
        return RotateRight(value, shift_amount);
    case ShiftType::RRX:
        return (value >> 1) | (carry << 31);
    default:
        assert(false);
        return 0;
    }
}

Cpu::ResultWithCarry Cpu::Shift_C(u32 value, ShiftType type, int shift_amount, u32 carry) {
    assert(!(type == ShiftType::RRX && shift_amount != 1));

    if (shift_amount == 0) {
        return {value, carry};
    }

    switch (type) {
    case ShiftType::LSL:
        return LogicalShiftLeft_C(value, shift_amount);
    case ShiftType::LSR:
        return LogicalShiftRight_C(value, shift_amount);
    case ShiftType::ASR:
        return ArithmeticShiftRight_C(value, shift_amount);
    case ShiftType::ROR:
        return RotateRight_C(value, shift_amount);
    case ShiftType::RRX:
        return RotateRightExtend_C(value, carry);
    default:
        assert(false);
        return {0, 0};
    }
}

Cpu::ResultWithCarry Cpu::LogicalShiftLeft_C(u32 value, int shift_amount) {
    u32 carry_out = (value << (shift_amount - 1)) >> 31;

    return {value << shift_amount, carry_out};
}

Cpu::ResultWithCarry Cpu::LogicalShiftRight_C(u32 value, int shift_amount) {
    u32 carry_out = (value >> (shift_amount - 1)) & 0x1;

    return {value >> shift_amount, carry_out};
}

Cpu::ResultWithCarry Cpu::ArithmeticShiftRight_C(u32 value, int shift_amount) {
    u32 carry_out = (static_cast<s32>(value) >> (shift_amount - 1)) & 0x1;

    return {static_cast<u32>(static_cast<s32>(value) >> shift_amount), carry_out};
}

Cpu::ResultWithCarry Cpu::RotateRight_C(u32 value, int shift_amount) {
    u32 result = RotateRight(value, shift_amount);

    return {result, result >> 31};
}

Cpu::ResultWithCarry Cpu::RotateRightExtend_C(u32 value, u32 carry_in) {
    return {(value >> 1) | (carry_in << 31), value & 0x1};
}

bool Cpu::ConditionPassed(Condition cond) const {
    switch (cond) {
    case Condition::Equal:         return GetZero();
    case Condition::NotEqual:      return !GetZero();
    case Condition::CarrySet:      return GetCarry();
    case Condition::CarryClear:    return !GetCarry();
    case Condition::Minus:         return GetSign();
    case Condition::Plus:          return !GetSign();
    case Condition::OverflowSet:   return GetOverflow();
    case Condition::OverflowClear: return !GetOverflow();
    case Condition::Higher:        return GetCarry() && !GetZero();
    case Condition::LowerSame:     return !(GetCarry() && !GetZero());
    case Condition::GreaterEqual:  return GetSign() == GetOverflow();
    case Condition::LessThan:      return !(GetSign() == GetOverflow());
    case Condition::GreaterThan:   return GetSign() == GetOverflow() && !GetZero();
    case Condition::LessEqual:     return !(GetSign() == GetOverflow() && !GetZero());
    default:                       return true;
    }
}

void Cpu::SetAllFlags(u64 result) {
    SetSign(result & sign_bit);
    SetZero(result == 0);
    SetCarry(result & carry_bit);
    SetOverflow(((result & carry_bit) >> 1) ^ (result & sign_bit));
}

void Cpu::SetSignZeroCarryFlags(u32 result, u32 carry) {
    SetSign(result & sign_bit);
    SetZero(result == 0);
    SetCarry(carry);
}

void Cpu::SetSignZeroFlags(u32 result) {
    SetSign(result & sign_bit);
    SetZero(result == 0);
}

void Cpu::ConditionalSetAllFlags(bool set_flags, u64 result) {
    if (set_flags) {
        SetAllFlags(result);
    }
}

void Cpu::ConditionalSetSignZeroCarryFlags(bool set_flags, u32 result, u32 carry) {
    if (set_flags) {
        SetSignZeroCarryFlags(result, carry);
    }
}

void Cpu::ConditionalSetSignZeroFlags(bool set_flags, u32 result) {
    if (set_flags) {
        SetSignZeroFlags(result);
    }
}

void Cpu::ConditionalSetMultiplyLongFlags(bool set_flags, u64 result) {
    if (set_flags) {
        SetSign(result & (1l << 63));
        SetZero(result == 0);
        SetCarry(0);
        SetOverflow(0);
    }
}

} // End namespace Gba
