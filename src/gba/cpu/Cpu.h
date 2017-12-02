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

#pragma once

#include <array>
#include <vector>
#include <functional>
#include <cassert>

#include "common/CommonTypes.h"
#include "common/CommonFuncs.h"

namespace Gba {

class Memory;

template<typename T>
class Instruction;

class Cpu {
public:
    Cpu(Memory& memory);
    ~Cpu();

    void Execute(int cycles);
private:
    Memory& mem;

    std::array<u32, 16> regs{};
    u32 cpsr = irq_disable | fiq_disable | svc;

    const std::vector<Instruction<Thumb>> thumb_instructions;
    const std::vector<Instruction<Arm>> arm_instructions;

    // Constants
    using Reg = std::size_t;
    static constexpr Reg sp = 13, lr = 14, pc = 15;
    static constexpr u64 carry_bit = 0x1'0000'0000, sign_bit = 0x8000'0000;

    enum CpsrMasks : u32 {sign        = 0x8000'0000,
                          zero        = 0x4000'0000,
                          carry       = 0x2000'0000,
                          overflow    = 0x1000'0000,
                          irq_disable = 0x0000'0080,
                          fiq_disable = 0x0000'0040,
                          thumb_mode  = 0x0000'0020,
                          mode        = 0x0000'001F};

    enum CpuMode : u32 {user   = 0x10,
                        fiq    = 0x11,
                        irq    = 0x12,
                        svc    = 0x13,
                        abort  = 0x17,
                        undef  = 0x1B,
                        system = 0x1F};

    enum class Condition {Equal         = 0b0000,
                          NotEqual      = 0b0001,
                          CarrySet      = 0b0010,
                          CarryClear    = 0b0011,
                          Minus         = 0b0100,
                          Plus          = 0b0101,
                          OverflowSet   = 0b0110,
                          OverflowClear = 0b0111,
                          Higher        = 0b1000,
                          LowerSame     = 0b1001,
                          GreaterEqual  = 0b1010,
                          LessThan      = 0b1011,
                          GreaterThan   = 0b1100,
                          LessEqual     = 0b1101,
                          Always        = 0b1110};

    enum class ShiftType {LSL = 0,
                          LSR = 1,
                          ASR = 2,
                          ROR = 3,
                          RRX = 4};

    // Types
    struct ResultWithCarry {
        u32 result;
        u32 carry;
    };

    struct ImmediateShift {
        ShiftType type;
        u32 imm;
    };

    // Functions
    bool ThumbMode() const { return cpsr & thumb_mode; }
    bool ArmMode() const { return !(cpsr & thumb_mode); }

    void SetSign(bool val)     { (val) ? (cpsr |= sign)     : (cpsr &= ~sign); }
    void SetZero(bool val)     { (val) ? (cpsr |= zero)     : (cpsr &= ~zero); }
    void SetCarry(bool val)    { (val) ? (cpsr |= carry)    : (cpsr &= ~carry); }
    void SetOverflow(bool val) { (val) ? (cpsr |= overflow) : (cpsr &= ~overflow); }

    u32 GetSign()     const { return (cpsr & sign)     >> 31; }
    u32 GetZero()     const { return (cpsr & zero)     >> 30; }
    u32 GetCarry()    const { return (cpsr & carry)    >> 29; }
    u32 GetOverflow() const { return (cpsr & overflow) >> 28; }

    std::function<int(Cpu& cpu, Thumb opcode)> DecodeThumb(Thumb opcode) const;
    std::function<int(Cpu& cpu, Arm opcode)> DecodeArm(Arm opcode) const;

    // ARM primitives
    static constexpr u32 ArmExpandImmediate(u32 value) noexcept {
        const u32 base_value = value & 0xFF;
        const u32 rotation = 2 * ((value >> 8) & 0x0F);

        return RotateRight(base_value, rotation);
    }

    static constexpr ResultWithCarry ArmExpandImmediate_C(u32 value) noexcept {
        const u32 result = ArmExpandImmediate(value);

        return {result, result >> 31};
    }

    static ImmediateShift DecodeImmShift(ShiftType type, u32 imm5);
    static u32 Shift(u32 value, ShiftType type, int shift_amount, u32 carry);
    static ResultWithCarry Shift_C(u32 value, ShiftType type, int shift_amount, u32 carry);
    static ResultWithCarry LogicalShiftLeft_C(u32 value, int shift_amount);
    static ResultWithCarry LogicalShiftRight_C(u32 value, int shift_amount);
    static ResultWithCarry ArithmeticShiftRight_C(u32 value, int shift_amount);
    static ResultWithCarry RotateRight_C(u32 value, int shift_amount);
    static ResultWithCarry RotateRightExtend_C(u32 value, u32 carry_in);

    static constexpr u64 AddWithCarry(u32 value1, u32 value2, u32 carry) {
        return static_cast<u64>(value1) + static_cast<u64>(value2) + static_cast<u64>(carry);
    }

    void Thumb_BranchWritePC(u32 addr) {
        regs[pc] = addr & ~0x1;
    }

    void Arm_BranchWritePC(u32 addr) {
        regs[pc] = addr & ~0x3;
    }

    void BxWritePC(u32 addr) {
        if ((addr & 0b01) == 0b01) {
            // Switch to Thumb mode.
            cpsr |= thumb_mode;
            regs[pc] = addr & ~0x1;
        } else if ((addr & 0b10) == 0b00) {
            // Switch to Arm mode.
            cpsr &= ~thumb_mode;
            regs[pc] = addr;
        } else {
            // Unpredictable if the lower 2 bits of the address are 0b10.
            assert(false);
        }
    }

    void SetAllFlags(u64 result);
    void SetSignZeroCarryFlags(u32 result, u32 carry);
    void SetSignZeroFlags(u32 result);
    void ConditionalSetAllFlags(bool set_flags, u64 result);
    void ConditionalSetSignZeroCarryFlags(bool set_flags, u32 result, u32 carry);
    void ConditionalSetSignZeroFlags(bool set_flags, u32 result);
    void ConditionalSetMultiplyLongFlags(bool set_flags, u64 result);

    bool ConditionPassed(Condition cond) const;

public:
    // ******************* Thumb Operators *******************

    // Arithmetic Operators
    int Thumb_AdcReg(Reg m, Reg d);

    int Thumb_AddImmT1(u32 imm, Reg n, Reg d);
    int Thumb_AddImmT2(Reg d, u32 imm);
    int Thumb_AddRegT1(Reg m, Reg n, Reg d);
    int Thumb_AddRegT2(Reg d1, Reg m, Reg d2);

    int Thumb_AddSpImmT1(Reg d, u32 imm);
    int Thumb_AddSpImmT2(u32 imm);
    int Thumb_AddPcImm(Reg d, u32 imm);

    int Thumb_CmnReg(Reg m, Reg n);

    int Thumb_CmpImm(Reg n, u32 imm);
    int Thumb_CmpRegT1(Reg m, Reg n);
    int Thumb_CmpRegT2(Reg n1, Reg m, Reg n2);

    int Thumb_MulReg(Reg n, Reg d);

    int Thumb_RsbImm(Reg n, Reg d);

    int Thumb_SbcReg(Reg m, Reg d);

    int Thumb_SubImmT1(u32 imm, Reg n, Reg d);
    int Thumb_SubImmT2(Reg d, u32 imm);
    int Thumb_SubReg(Reg m, Reg n, Reg d);

    int Thumb_SubSpImm(u32 imm);

    // Logical Operators
    int Thumb_AndReg(Reg m, Reg d);

    int Thumb_BicReg(Reg m, Reg d);

    int Thumb_EorReg(Reg m, Reg d);

    int Thumb_OrrReg(Reg m, Reg d);

    int Thumb_TstReg(Reg m, Reg n);

    // Shifts
    int Thumb_AsrImm(u32 imm, Reg m, Reg d);
    int Thumb_AsrReg(Reg m, Reg d);

    int Thumb_LslImm(u32 imm, Reg m, Reg d);
    int Thumb_LslReg(Reg m, Reg d);

    int Thumb_LsrImm(u32 imm, Reg m, Reg d);
    int Thumb_LsrReg(Reg m, Reg d);

    int Thumb_RorReg(Reg m, Reg d);

    // Branches
    int Thumb_BT1(Condition cond, u32 imm8);
    int Thumb_BT2(u32 imm11);

    int Thumb_BlH1(u32 imm11);
    int Thumb_BlH2(u32 imm11);

    int Thumb_Bx(Reg m);

    // Moves
    int Thumb_MovImm(Reg d, u32 imm);
    int Thumb_MovRegT1(Reg d1, Reg m, Reg d2);
    int Thumb_MovRegT2(Reg m, Reg d);

    int Thumb_MvnReg(Reg m, Reg d);

    // Loads
    int Thumb_Ldm(Reg n, u32 reg_list);

    int Thumb_LdrImm(u32 imm, Reg n, Reg t);
    int Thumb_LdrSpImm(Reg t, u32 imm);
    int Thumb_LdrPcImm(Reg t, u32 imm);
    int Thumb_LdrReg(Reg m, Reg n, Reg t);

    int Thumb_LdrbImm(u32 imm, Reg n, Reg t);
    int Thumb_LdrbReg(Reg m, Reg n, Reg t);

    int Thumb_LdrhImm(u32 imm, Reg n, Reg t);
    int Thumb_LdrhReg(Reg m, Reg n, Reg t);

    int Thumb_LdrsbReg(Reg m, Reg n, Reg t);
    int Thumb_LdrshReg(Reg m, Reg n, Reg t);

    int Thumb_Pop(bool p, u32 reg_list);

    // Stores
    int Thumb_Push(bool m, u32 reg_list);

    int Thumb_Stm(Reg n, u32 reg_list);

    int Thumb_StrImm(u32 imm, Reg n, Reg t);
    int Thumb_StrSpImm(Reg t, u32 imm);
    int Thumb_StrReg(Reg m, Reg n, Reg t);

    int Thumb_StrbImm(u32 imm, Reg n, Reg t);
    int Thumb_StrbReg(Reg m, Reg n, Reg t);

    int Thumb_StrhImm(u32 imm, Reg n, Reg t);
    int Thumb_StrhReg(Reg m, Reg n, Reg t);

    // Misc
    int Thumb_Swi(u32 imm);

    int Thumb_Undefined(u16 opcode);

    // ******************* ARM Operators *******************

    // Arithmetic Operators
    int Arm_AdcImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm);
    int Arm_AdcReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    int Arm_AdcRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    int Arm_AddImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm);
    int Arm_AddReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    int Arm_AddRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    int Arm_CmnImm(Condition cond, Reg n, u32 imm);
    int Arm_CmnReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m);
    int Arm_CmnRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m);

    int Arm_CmpImm(Condition cond, Reg n, u32 imm);
    int Arm_CmpReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m);
    int Arm_CmpRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m);

    int Arm_MlaReg(Condition cond, bool set_flags, Reg d, Reg a, Reg m, Reg n);
    int Arm_MulReg(Condition cond, bool set_flags, Reg d, Reg m, Reg n);

    int Arm_RsbImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm);
    int Arm_RsbReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    int Arm_RsbRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    int Arm_RscImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm);
    int Arm_RscReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    int Arm_RscRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    int Arm_SbcImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm);
    int Arm_SbcReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    int Arm_SbcRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    int Arm_SmlalReg(Condition cond, bool set_flags, Reg dh, Reg dl, Reg m, Reg n);
    int Arm_SmullReg(Condition cond, bool set_flags, Reg dh, Reg dl, Reg m, Reg n);

    int Arm_SubImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm);
    int Arm_SubReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    int Arm_SubRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    int Arm_UmlalReg(Condition cond, bool set_flags, Reg dh, Reg dl, Reg m, Reg n);
    int Arm_UmullReg(Condition cond, bool set_flags, Reg dh, Reg dl, Reg m, Reg n);

    // Logical Operators
    int Arm_AndImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm);
    int Arm_AndReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    int Arm_AndRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    int Arm_BicImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm);
    int Arm_BicReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    int Arm_BicRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    int Arm_EorImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm);
    int Arm_EorReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    int Arm_EorRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    int Arm_OrrImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm);
    int Arm_OrrReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    int Arm_OrrRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    int Arm_TeqImm(Condition cond, Reg n, u32 imm);
    int Arm_TeqReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m);
    int Arm_TeqRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m);

    int Arm_TstImm(Condition cond, Reg n, u32 imm);
    int Arm_TstReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m);
    int Arm_TstRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m);

    // Shifts
    int Arm_AsrImm(Condition cond, bool set_flags, Reg d, u32 imm, Reg m);
    int Arm_AsrReg(Condition cond, bool set_flags, Reg d, Reg m, Reg n);

    int Arm_LslImm(Condition cond, bool set_flags, Reg d, u32 imm, Reg m);
    int Arm_LslReg(Condition cond, bool set_flags, Reg d, Reg m, Reg n);

    int Arm_LsrImm(Condition cond, bool set_flags, Reg d, u32 imm, Reg m);
    int Arm_LsrReg(Condition cond, bool set_flags, Reg d, Reg m, Reg n);

    int Arm_RorImm(Condition cond, bool set_flags, Reg d, u32 imm, Reg m);
    int Arm_RorReg(Condition cond, bool set_flags, Reg d, Reg m, Reg n);

    // Branches
    int Arm_B(Condition cond, u32 imm24);

    int Arm_Bl(Condition cond, u32 imm24);

    int Arm_Bx(Condition cond, Reg m);

    // Moves
    int Arm_MovImm(Condition cond, bool set_flags, Reg d, u32 imm);
    int Arm_MovReg(Condition cond, bool set_flags, Reg d, Reg m);

    int Arm_MvnImm(Condition cond, bool set_flags, Reg d, u32 imm);
    int Arm_MvnReg(Condition cond, bool set_flags, Reg d, u32 imm, ShiftType type, Reg m);
    int Arm_MvnRegShifted(Condition cond, bool set_flags, Reg d, Reg s, ShiftType type, Reg m);

    // Loads
    int Arm_Ldmi(Condition cond, bool pre_indexed, bool writeback, Reg n, u32 reg_list);
    int Arm_Ldmd(Condition cond, bool pre_indexed, bool writeback, Reg n, u32 reg_list);

    int Arm_LdrImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm);
    int Arm_LdrReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm,
                   ShiftType type, Reg m);

    int Arm_LdrbImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm);
    int Arm_LdrbReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm,
                    ShiftType type, Reg m);

    int Arm_LdrhImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm_hi, u32 imm_lo);
    int Arm_LdrhReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, Reg m);

    int Arm_LdrsbImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm_hi, u32 imm_lo);
    int Arm_LdrsbReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, Reg m);

    int Arm_LdrshImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm_hi, u32 imm_lo);
    int Arm_LdrshReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, Reg m);

    int Arm_PopA1(Condition cond, u32 reg_list);
    int Arm_PopA2(Condition cond, Reg t);

    // Stores
    int Arm_PushA1(Condition cond, u32 reg_list);
    int Arm_PushA2(Condition cond, Reg t);

    int Arm_Stmi(Condition cond, bool pre_indexed, bool writeback, Reg n, u32 reg_list);
    int Arm_Stmd(Condition cond, bool pre_indexed, bool writeback, Reg n, u32 reg_list);

    int Arm_StrImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm);
    int Arm_StrReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm,
                   ShiftType type, Reg m);

    int Arm_StrbImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm);
    int Arm_StrbReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm,
                    ShiftType type, Reg m);

    int Arm_StrhImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm_hi, u32 imm_lo);
    int Arm_StrhReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, Reg m);

    int Arm_SwpReg(Condition cond, bool byte, Reg n, Reg t1, Reg t2);

    // Misc
    int Arm_Cdp(Condition cond, u32 opcode1, Reg cn, Reg cd, u32 coproc, u32 opcode2, Reg cm);
    int Arm_Ldc(Condition cond, bool p, bool u, bool d, bool w, Reg n, Reg cd, u32 coproc, u32 imm);
    int Arm_Mcr(Condition cond, u32 opcode1, Reg cn, Reg t, u32 coproc, u32 opcode2, Reg cm);

    int Arm_Mrs(Condition cond, bool read_spsr, Reg d);

    int Arm_MsrImm(Condition cond, bool read_spsr, u32 mask, u32 imm);
    int Arm_MsrReg(Condition cond, bool read_spsr, u32 mask, Reg n);

    int Arm_Swi(Condition cond, u32 imm);

    int Arm_Undefined(u32 opcode);
};

} // End namespace Gba
