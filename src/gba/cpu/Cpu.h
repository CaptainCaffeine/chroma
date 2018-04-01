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

#pragma once

#include <array>
#include <vector>
#include <functional>
#include <memory>
#include <tuple>

#include "common/CommonTypes.h"
#include "common/CommonFuncs.h"
#include "common/CommonEnums.h"
#include "gba/core/Enums.h"

namespace Gba {

class Memory;
class Disassembler;
class Core;

template<typename T>
class Instruction;

// Declared outside of class for Disassembler.
struct ImmediateShift {
    ShiftType type;
    u32 imm;
};

class Cpu {
public:
    Cpu(Memory& _mem, Core& _core, LogLevel level);
    ~Cpu();

    std::unique_ptr<Disassembler> disasm;

    bool dma_active = false;
    u32 last_bios_fetch = 0x0;

    int Execute(int cycles);
    void Halt() { halted = true; }

    u32 GetPc() const { return regs[pc]; };
    u32 GetPrefetchedOpcode(int i) const { return pipeline[i]; }

    bool ThumbMode() const { return cpsr & thumb_mode; }
    bool ArmMode() const { return !(cpsr & thumb_mode); }

    // Public for Disassembler.
    static ImmediateShift DecodeImmShift(ShiftType type, u32 imm5);
    static constexpr u32 ArmExpandImmediate(u32 value) noexcept {
        const u32 base_value = value & 0xFF;
        const u32 rotation = 2 * ((value >> 8) & 0x0F);

        return RotateRight(base_value, rotation);
    }

private:
    Memory& mem;
    Core& core;

    std::array<u32, 16> regs{};
    u32 cpsr = irq_disable | fiq_disable | static_cast<u32>(CpuMode::Svc);

    std::array<u32, 16> spsr{};
    std::array<u32, 16> sp_banked{};
    std::array<u32, 16> lr_banked{};
    std::array<u32, 5> fiq_banked_regs{};

    const std::vector<Instruction<Thumb>> thumb_instructions;
    const std::vector<Instruction<Arm>> arm_instructions;
    std::array<const std::function<int(Cpu& cpu, Thumb opcode)> *, 0x400> thumb_decode_table;

    std::array<u32, 3> pipeline{};
    bool pc_written = false;

    bool halted = false;

    // Constants
    using Reg = std::size_t;
    static constexpr Reg sp = 13, lr = 14, pc = 15;
    static constexpr u64 carry_bit = 0x1'0000'0000, sign_bit = 0x8000'0000;

    enum CpsrMasks : u32 {sign_flag     = 0x8000'0000,
                          zero_flag     = 0x4000'0000,
                          carry_flag    = 0x2000'0000,
                          overflow_flag = 0x1000'0000,
                          irq_disable   = 0x0000'0080,
                          fiq_disable   = 0x0000'0040,
                          thumb_mode    = 0x0000'0020,
                          cpu_mode      = 0x0000'001F};

    enum class CpuMode : u32 {User   = 0x10,
                              Fiq    = 0x11,
                              Irq    = 0x12,
                              Svc    = 0x13,
                              Abort  = 0x17,
                              Undef  = 0x1B,
                              System = 0x1F};

    // Types
    struct ResultWithCarry {
        u32 result;
        u32 carry;
    };

    struct ArithResult {
        u64 value;
        bool overflow;
    };

    using ArithOp = ArithResult(*)(u32,u32,u32);
    using MullOp = s64(*)(u32,u32,u32,u32);
    using LogicOp = u32(*)(u32,u32);
    using LoadOp = std::tuple<u32,int>(*)(Memory&,u32);
    using StoreOp = int(*)(Memory&,u32,u32);

    // Functions
    bool InterruptsEnabled() const;

    CpuMode CurrentCpuMode() const { return static_cast<CpuMode>(cpsr & cpu_mode); }
    // Since bit 4 is 1 for all valid CPU modes, we ignore it when indexing banked registers.
    std::size_t CurrentCpuModeIndex() const { return cpsr & 0xF; }
    std::size_t CpuModeIndex(CpuMode mode) const { return static_cast<u32>(mode) & 0xF; }

    bool HasSpsr() const { return CurrentCpuMode() != CpuMode::User && CurrentCpuMode() != CpuMode::System; }
    bool ValidCpuMode(u32 new_mode) const;
    void CpuModeSwitch(CpuMode new_cpu_mode);

    int FlushPipeline();

    int TakeException(CpuMode exception_type);
    int ReturnFromException(u32 address);

    void InternalCycle(int cycles);

    void SetSign(bool val)     { (val) ? (cpsr |= sign_flag)     : (cpsr &= ~sign_flag); }
    void SetZero(bool val)     { (val) ? (cpsr |= zero_flag)     : (cpsr &= ~zero_flag); }
    void SetCarry(bool val)    { (val) ? (cpsr |= carry_flag)    : (cpsr &= ~carry_flag); }
    void SetOverflow(bool val) { (val) ? (cpsr |= overflow_flag) : (cpsr &= ~overflow_flag); }

    u32 GetSign()     const { return (cpsr & sign_flag)     >> 31; }
    u32 GetZero()     const { return (cpsr & zero_flag)     >> 30; }
    u32 GetCarry()    const { return (cpsr & carry_flag)    >> 29; }
    u32 GetOverflow() const { return (cpsr & overflow_flag) >> 28; }

    void PopulateThumbDecodeTable();
    const std::function<int(Cpu& cpu, Thumb opcode)>& DecodeThumb(Thumb opcode) const;
    const std::function<int(Cpu& cpu, Arm opcode)>& DecodeArm(Arm opcode) const;

    // ARM primitives
    static constexpr ResultWithCarry ArmExpandImmediate_C(u32 value) noexcept {
        const u32 result = ArmExpandImmediate(value);

        return {result, result >> 31};
    }

    u32 Shift(u32 value, ShiftType type, int shift_amount);
    ResultWithCarry Shift_C(u32 value, ShiftType type, int shift_amount);
    static ResultWithCarry LogicalShiftLeft_C(u64 value, int shift_amount);
    static ResultWithCarry LogicalShiftRight_C(u64 value, int shift_amount);
    static ResultWithCarry ArithmeticShiftRight_C(s32 value, int shift_amount);
    static ResultWithCarry RotateRight_C(u32 value, int shift_amount);
    static ResultWithCarry RotateRightExtend_C(u32 value, u32 carry_in);

    static constexpr ArithResult AddWithCarry(u64 value1, u64 value2, u64 carry) {
        u64 result = value1 + value2 + carry;
        bool overflow = (value1 & sign_bit) == (value2 & sign_bit) && (value1 & sign_bit) != (result & sign_bit);
        return {result, overflow};
    }

    int Thumb_BranchWritePC(u32 addr) {
        regs[pc] = addr & ~0x1;
        return FlushPipeline();
    }

    int Arm_BranchWritePC(u32 addr) {
        regs[pc] = addr & ~0x3;
        return FlushPipeline();
    }

    int BxWritePC(u32 addr);

    void SetAllFlags(ArithResult result);
    void SetSignZeroCarryFlags(u32 result, u32 carry);
    void SetSignZeroFlags(u32 result);
    void ConditionalSetAllFlags(bool set_flags, ArithResult result);
    void ConditionalSetSignZeroCarryFlags(bool set_flags, u32 result, u32 carry);
    void ConditionalSetSignZeroFlags(bool set_flags, u32 result);
    void ConditionalSetMultiplyLongFlags(bool set_flags, u64 result);

    bool ConditionPassed(Condition cond) const;

    static int MultiplyCycles(u32 operand);

    // Implementation Helpers
    ArithOp add_op = [](u32 value1, u32 value2, u32 carry) { return AddWithCarry(value1, value2, carry); };
    ArithOp sub_op = [](u32 value1, u32 value2, u32 carry) { return AddWithCarry(value1, ~value2, carry); };
    ArithOp rsb_op = [](u32 value1, u32 value2, u32 carry) { return AddWithCarry(~value1, value2, carry); };

    LogicOp and_op = [](u32 value1, u32 value2) { return value1 & value2; };
    LogicOp bic_op = [](u32 value1, u32 value2) { return value1 & ~value2; };
    LogicOp eor_op = [](u32 value1, u32 value2) { return value1 ^ value2; };
    LogicOp orr_op = [](u32 value1, u32 value2) { return value1 | value2; };
    LogicOp mvn_op = [](u32, u32 value) { return ~value; };

    int Thumb_ArithImm(u32 imm, Reg n, Reg d, ArithOp op, u32 carry);
    int Thumb_ArithReg(Reg m, Reg n, Reg d, ArithOp op, u32 carry);
    int Thumb_ArithImmSp(Reg d, u32 imm, ArithOp op, u32 carry);

    int Thumb_Compare(u32 imm, Reg n, ArithOp op, u32 carry);

    int Thumb_LogicReg(Reg m, Reg d, LogicOp op);

    int Thumb_ShiftImm(u32 imm, Reg m, Reg d, ShiftType type);
    int Thumb_ShiftReg(Reg m, Reg d, ShiftType type);

    int Thumb_Load(u32 imm, Reg n, Reg t, LoadOp op);
    int Thumb_Store(u32 imm, Reg n, Reg t, StoreOp op);

    int AluWritePC(bool set_flags, u32 result);

    int Arm_ArithImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ArithOp op, u32 carry);
    int Arm_ArithReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m, ArithOp op,
                     u32 carry);
    int Arm_ArithRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m, ArithOp op,
                            u32 carry);

    int Arm_CompareImm(Condition cond, Reg n, u32 imm, ArithOp op, u32 carry);
    int Arm_CompareReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m, ArithOp op, u32 carry);
    int Arm_CompareRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m, ArithOp op, u32 carry);

    int Arm_MultiplyReg(Condition cond, bool set_flags, Reg d, Reg a, Reg m, Reg n);
    int Arm_MultiplyLongReg(Condition cond, bool set_flags, Reg dh, Reg dl, Reg m, Reg n, MullOp op, bool acc);

    int Arm_LogicImm(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, LogicOp op);
    int Arm_LogicReg(Condition cond, bool set_flags, Reg n, Reg d, u32 imm, ShiftType type, Reg m, LogicOp op);
    int Arm_LogicRegShifted(Condition cond, bool set_flags, Reg n, Reg d, Reg s, ShiftType type, Reg m, LogicOp op);

    int Arm_TestImm(Condition cond, Reg n, u32 imm, LogicOp op);
    int Arm_TestReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m, LogicOp op);
    int Arm_TestRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m, LogicOp op);

    int Arm_ShiftImm(Condition cond, bool set_flags, Reg d, u32 imm, Reg m, ShiftType type);
    int Arm_ShiftReg(Condition cond, bool set_flags, Reg d, Reg m, Reg n, ShiftType type);

    int Arm_LoadImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm, LoadOp op);
    int Arm_LoadReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm,
                    ShiftType type, Reg m, LoadOp op);

    int Arm_StoreImm(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm, StoreOp op);
    int Arm_StoreReg(Condition cond, bool pre_indexed, bool add, bool writeback, Reg n, Reg t, u32 imm,
                     ShiftType type, Reg m, StoreOp op);

    int Arm_WriteStatusReg(Condition cond, bool write_spsr, u32 mask, u32 value);

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
    int Arm_Ldm(Condition cond, bool pre_indexed, bool increment, bool exception_return, bool writeback, Reg n,
                u32 reg_list);

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

    int Arm_Stm(Condition cond, bool pre_indexed, bool increment, bool store_user_regs, bool writeback, Reg n,
                u32 reg_list);

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

    int Arm_MsrImm(Condition cond, bool write_spsr, u32 mask, u32 imm);
    int Arm_MsrReg(Condition cond, bool write_spsr, u32 mask, Reg n);

    int Arm_Swi(Condition cond, u32 imm);

    int Arm_Undefined(u32 opcode);
};

} // End namespace Gba
