// This file is a part of Chroma.
// Copyright (C) 2018-2019 Matthew Murray
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

#include <vector>
#include <string>
#include <fstream>
#include <utility>
#include <fmt/ostream.h>

#include "common/CommonTypes.h"
#include "common/CommonFuncs.h"
#include "common/CommonEnums.h"
#include "gba/cpu/CpuDefs.h"

namespace Gba {

class Core;

template<typename T, typename D>
class Instruction;

class Disassembler {
public:
    Disassembler(LogLevel level, Core& _core);
    ~Disassembler();

    // Return type for Instruction impl functions.
    using ReturnType = std::string;

    void DisassembleThumb(Thumb opcode, const std::array<u32, 16>& regs, u32 cpsr);
    void DisassembleArm(Arm opcode, const std::array<u32, 16>& regs, u32 cpsr);

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

    void SwitchLogLevel();

private:
    Core& core;

    const std::vector<Instruction<Thumb, Disassembler>> thumb_instructions;
    const std::vector<Instruction<Arm, Disassembler>> arm_instructions;

    LogLevel log_level = LogLevel::None;
    LogLevel alt_level;
    std::ofstream log_stream;

    int halt_cycles = 0;

    static std::string Flags(bool sf) { return (sf) ? "S" : ""; }
    static std::string RegStr(Reg r);
    static std::string ShiftStr(ImmediateShift shift);
    static std::string ListStr(u32 reg_list);
    static std::string AddrOffset(bool pre_indexed, bool add, bool wb, u32 imm);
    static std::string StatusReg(bool spsr, u32 mask);

    void LogRegisters(const std::array<u32, 16>& regs, u32 cpsr);

    // Arm
    std::string AluImm(const char* name, Condition cond, bool sf, Reg n, Reg d, u32 imm);
    std::string AluReg(const char* name, Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    std::string AluRegShifted(const char* name, Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    std::string FlagsImm(const char* name, Condition cond, bool sf, Reg n, u32 imm);
    std::string FlagsReg(const char* name, Condition cond, bool sf, Reg n, u32 imm, ShiftType type, Reg m);
    std::string FlagsRegShifted(const char* name, Condition cond, bool sf, Reg n, Reg s, ShiftType type, Reg m);

    std::string MultiplyLong(const char* name, Condition cond, bool sf, Reg h, Reg l, Reg m, Reg n);

    std::string ShiftImm(ShiftType type, Condition cond, bool sf, Reg d, u32 imm, Reg m);
    std::string ShiftReg(const char* name, Condition cond, bool sf, Reg d, Reg m, Reg n);

    std::string BranchImm(const char* name, Condition cond, s32 signed_imm32);

    std::string LoadMultiple(const char* name, Condition cond, bool pre_indexed, bool increment, bool exception_return,
                             bool wb, Reg n, u32 reg_list);

    std::string LoadImm(const char* name, Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm);
    std::string LoadReg(const char* name, Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm,
                        ShiftType type, Reg m);

    // Thumb
    std::string AluImm(const char* name, Reg d, u32 imm);
    std::string AluRegImm(const char* name, u32 imm, Reg n, Reg d);
    std::string AluReg(const char* name, Reg m, Reg d);
    std::string AluRegReg(const char* name, Reg m, Reg n, Reg d);

    std::string LoadImm(const char* name, u32 imm, Reg n, Reg t);
    std::string LoadReg(const char* name, Reg m, Reg n, Reg t);

public:
    // ******************* Thumb Operators *******************

    // Arithmetic Operators
    std::string Thumb_AdcReg(Reg m, Reg d);

    std::string Thumb_AddImmT1(u32 imm, Reg n, Reg d);
    std::string Thumb_AddImmT2(Reg d, u32 imm);
    std::string Thumb_AddRegT1(Reg m, Reg n, Reg d);
    std::string Thumb_AddRegT2(Reg d1, Reg m, Reg d2);

    std::string Thumb_AddSpImmT1(Reg d, u32 imm);
    std::string Thumb_AddSpImmT2(u32 imm);
    std::string Thumb_AddPcImm(Reg d, u32 imm);

    std::string Thumb_CmnReg(Reg m, Reg n);

    std::string Thumb_CmpImm(Reg n, u32 imm);
    std::string Thumb_CmpRegT1(Reg m, Reg n);
    std::string Thumb_CmpRegT2(Reg n1, Reg m, Reg n2);

    std::string Thumb_MulReg(Reg n, Reg d);

    std::string Thumb_RsbImm(Reg n, Reg d);

    std::string Thumb_SbcReg(Reg m, Reg d);

    std::string Thumb_SubImmT1(u32 imm, Reg n, Reg d);
    std::string Thumb_SubImmT2(Reg d, u32 imm);
    std::string Thumb_SubReg(Reg m, Reg n, Reg d);

    std::string Thumb_SubSpImm(u32 imm);

    // Logical Operators
    std::string Thumb_AndReg(Reg m, Reg d);

    std::string Thumb_BicReg(Reg m, Reg d);

    std::string Thumb_EorReg(Reg m, Reg d);

    std::string Thumb_OrrReg(Reg m, Reg d);

    std::string Thumb_TstReg(Reg m, Reg n);

    // Shifts
    std::string Thumb_AsrImm(u32 imm, Reg m, Reg d);
    std::string Thumb_AsrReg(Reg m, Reg d);

    std::string Thumb_LslImm(u32 imm, Reg m, Reg d);
    std::string Thumb_LslReg(Reg m, Reg d);

    std::string Thumb_LsrImm(u32 imm, Reg m, Reg d);
    std::string Thumb_LsrReg(Reg m, Reg d);

    std::string Thumb_RorReg(Reg m, Reg d);

    // Branches
    std::string Thumb_BT1(Condition cond, u32 imm8);
    std::string Thumb_BT2(u32 imm11);

    std::string Thumb_BlH1(u32 imm11);
    std::string Thumb_BlH2(u32 imm11);

    std::string Thumb_Bx(Reg m);

    // Moves
    std::string Thumb_MovImm(Reg d, u32 imm);
    std::string Thumb_MovRegT1(Reg d1, Reg m, Reg d2);
    std::string Thumb_MovRegT2(Reg m, Reg d);

    std::string Thumb_MvnReg(Reg m, Reg d);

    // Loads
    std::string Thumb_Ldm(Reg n, u32 reg_list);

    std::string Thumb_LdrImm(u32 imm, Reg n, Reg t);
    std::string Thumb_LdrSpImm(Reg t, u32 imm);
    std::string Thumb_LdrPcImm(Reg t, u32 imm);
    std::string Thumb_LdrReg(Reg m, Reg n, Reg t);

    std::string Thumb_LdrbImm(u32 imm, Reg n, Reg t);
    std::string Thumb_LdrbReg(Reg m, Reg n, Reg t);

    std::string Thumb_LdrhImm(u32 imm, Reg n, Reg t);
    std::string Thumb_LdrhReg(Reg m, Reg n, Reg t);

    std::string Thumb_LdrsbReg(Reg m, Reg n, Reg t);
    std::string Thumb_LdrshReg(Reg m, Reg n, Reg t);

    std::string Thumb_Pop(bool p, u32 reg_list);

    // Stores
    std::string Thumb_Push(bool m, u32 reg_list);

    std::string Thumb_Stm(Reg n, u32 reg_list);

    std::string Thumb_StrImm(u32 imm, Reg n, Reg t);
    std::string Thumb_StrSpImm(Reg t, u32 imm);
    std::string Thumb_StrReg(Reg m, Reg n, Reg t);

    std::string Thumb_StrbImm(u32 imm, Reg n, Reg t);
    std::string Thumb_StrbReg(Reg m, Reg n, Reg t);

    std::string Thumb_StrhImm(u32 imm, Reg n, Reg t);
    std::string Thumb_StrhReg(Reg m, Reg n, Reg t);

    // Misc
    std::string Thumb_Swi(u32 imm);

    std::string Thumb_Undefined(u16 opcode);

    // ******************* ARM Operators *******************

    // Arithmetic Operators
    std::string Arm_AdcImm(Condition cond, bool sf, Reg n, Reg d, u32 imm);
    std::string Arm_AdcReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    std::string Arm_AdcRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    std::string Arm_AddImm(Condition cond, bool sf, Reg n, Reg d, u32 imm);
    std::string Arm_AddReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    std::string Arm_AddRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    std::string Arm_CmnImm(Condition cond, Reg n, u32 imm);
    std::string Arm_CmnReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m);
    std::string Arm_CmnRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m);

    std::string Arm_CmpImm(Condition cond, Reg n, u32 imm);
    std::string Arm_CmpReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m);
    std::string Arm_CmpRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m);

    std::string Arm_MlaReg(Condition cond, bool sf, Reg d, Reg a, Reg m, Reg n);
    std::string Arm_MulReg(Condition cond, bool sf, Reg d, Reg m, Reg n);

    std::string Arm_RsbImm(Condition cond, bool sf, Reg n, Reg d, u32 imm);
    std::string Arm_RsbReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    std::string Arm_RsbRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    std::string Arm_RscImm(Condition cond, bool sf, Reg n, Reg d, u32 imm);
    std::string Arm_RscReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    std::string Arm_RscRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    std::string Arm_SbcImm(Condition cond, bool sf, Reg n, Reg d, u32 imm);
    std::string Arm_SbcReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    std::string Arm_SbcRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    std::string Arm_SmlalReg(Condition cond, bool sf, Reg dh, Reg dl, Reg m, Reg n);
    std::string Arm_SmullReg(Condition cond, bool sf, Reg dh, Reg dl, Reg m, Reg n);

    std::string Arm_SubImm(Condition cond, bool sf, Reg n, Reg d, u32 imm);
    std::string Arm_SubReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    std::string Arm_SubRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    std::string Arm_UmlalReg(Condition cond, bool sf, Reg dh, Reg dl, Reg m, Reg n);
    std::string Arm_UmullReg(Condition cond, bool sf, Reg dh, Reg dl, Reg m, Reg n);

    // Logical Operators
    std::string Arm_AndImm(Condition cond, bool sf, Reg n, Reg d, u32 imm);
    std::string Arm_AndReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    std::string Arm_AndRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    std::string Arm_BicImm(Condition cond, bool sf, Reg n, Reg d, u32 imm);
    std::string Arm_BicReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    std::string Arm_BicRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    std::string Arm_EorImm(Condition cond, bool sf, Reg n, Reg d, u32 imm);
    std::string Arm_EorReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    std::string Arm_EorRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    std::string Arm_OrrImm(Condition cond, bool sf, Reg n, Reg d, u32 imm);
    std::string Arm_OrrReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m);
    std::string Arm_OrrRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m);

    std::string Arm_TeqImm(Condition cond, Reg n, u32 imm);
    std::string Arm_TeqReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m);
    std::string Arm_TeqRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m);

    std::string Arm_TstImm(Condition cond, Reg n, u32 imm);
    std::string Arm_TstReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m);
    std::string Arm_TstRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m);

    // Shifts
    std::string Arm_AsrImm(Condition cond, bool sf, Reg d, u32 imm, Reg m);
    std::string Arm_AsrReg(Condition cond, bool sf, Reg d, Reg m, Reg n);

    std::string Arm_LslImm(Condition cond, bool sf, Reg d, u32 imm, Reg m);
    std::string Arm_LslReg(Condition cond, bool sf, Reg d, Reg m, Reg n);

    std::string Arm_LsrImm(Condition cond, bool sf, Reg d, u32 imm, Reg m);
    std::string Arm_LsrReg(Condition cond, bool sf, Reg d, Reg m, Reg n);

    std::string Arm_RorImm(Condition cond, bool sf, Reg d, u32 imm, Reg m);
    std::string Arm_RorReg(Condition cond, bool sf, Reg d, Reg m, Reg n);

    // Branches
    std::string Arm_B(Condition cond, u32 imm24);
    std::string Arm_Bl(Condition cond, u32 imm24);
    std::string Arm_Bx(Condition cond, Reg m);

    // Moves
    std::string Arm_MovImm(Condition cond, bool sf, Reg d, u32 imm);
    std::string Arm_MovReg(Condition cond, bool sf, Reg d, Reg m);

    std::string Arm_MvnImm(Condition cond, bool sf, Reg d, u32 imm);
    std::string Arm_MvnReg(Condition cond, bool sf, Reg d, u32 imm, ShiftType type, Reg m);
    std::string Arm_MvnRegShifted(Condition cond, bool sf, Reg d, Reg s, ShiftType type, Reg m);

    // Loads
    std::string Arm_Ldm(Condition cond, bool pre_indexed, bool increment, bool exception_return, bool wb, Reg n,
                        u32 reg_list);

    std::string Arm_LdrImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm);
    std::string Arm_LdrReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm,
                           ShiftType type, Reg m);

    std::string Arm_LdrbImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm);
    std::string Arm_LdrbReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm,
                            ShiftType type, Reg m);

    std::string Arm_LdrhImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm_hi, u32 imm_lo);
    std::string Arm_LdrhReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, Reg m);

    std::string Arm_LdrsbImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm_hi, u32 imm_lo);
    std::string Arm_LdrsbReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, Reg m);

    std::string Arm_LdrshImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm_hi, u32 imm_lo);
    std::string Arm_LdrshReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, Reg m);

    std::string Arm_PopA1(Condition cond, u32 reg_list);
    std::string Arm_PopA2(Condition cond, Reg t);

    // Stores
    std::string Arm_PushA1(Condition cond, u32 reg_list);
    std::string Arm_PushA2(Condition cond, Reg t);

    std::string Arm_Stm(Condition cond, bool pre_indexed, bool increment, bool store_user_regs, bool wb, Reg n,
                        u32 reg_list);

    std::string Arm_StrImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm);
    std::string Arm_StrReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm,
                           ShiftType type, Reg m);

    std::string Arm_StrbImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm);
    std::string Arm_StrbReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm,
                            ShiftType type, Reg m);

    std::string Arm_StrhImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm_hi, u32 imm_lo);
    std::string Arm_StrhReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, Reg m);

    std::string Arm_SwpReg(Condition cond, bool byte, Reg n, Reg t1, Reg t2);

    // Misc
    std::string Arm_Cdp(Condition cond, u32 opcode1, Reg cn, Reg cd, u32 coproc, u32 opcode2, Reg cm);
    std::string Arm_Ldc(Condition cond, bool p, bool u, bool d, bool w, Reg n, Reg cd, u32 coproc, u32 imm);
    std::string Arm_Mcr(Condition cond, u32 opcode1, Reg cn, Reg t, u32 coproc, u32 opcode2, Reg cm);

    std::string Arm_Mrs(Condition cond, bool read_spsr, Reg d);

    std::string Arm_MsrImm(Condition cond, bool write_spsr, u32 mask, u32 imm);
    std::string Arm_MsrReg(Condition cond, bool write_spsr, u32 mask, Reg n);

    std::string Arm_Swi(Condition cond, u32 imm);

    std::string Arm_Undefined(u32 opcode);
};

} // End namespace Gba

namespace fmt {

template <>
struct formatter<Gba::Condition> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Gba::Condition &cond, FormatContext &ctx) {
        std::string cond_str;

        switch (cond) {
        case Gba::Condition::Equal:         cond_str = "EQ"; break;
        case Gba::Condition::NotEqual:      cond_str = "NE"; break;
        case Gba::Condition::CarrySet:      cond_str = "CS"; break;
        case Gba::Condition::CarryClear:    cond_str = "CC"; break;
        case Gba::Condition::Minus:         cond_str = "MI"; break;
        case Gba::Condition::Plus:          cond_str = "PL"; break;
        case Gba::Condition::OverflowSet:   cond_str = "VS"; break;
        case Gba::Condition::OverflowClear: cond_str = "VC"; break;
        case Gba::Condition::Higher:        cond_str = "HI"; break;
        case Gba::Condition::LowerSame:     cond_str = "LS"; break;
        case Gba::Condition::GreaterEqual:  cond_str = "GE"; break;
        case Gba::Condition::LessThan:      cond_str = "LT"; break;
        case Gba::Condition::GreaterThan:   cond_str = "GT"; break;
        case Gba::Condition::LessEqual:     cond_str = "LE"; break;
        default:                            cond_str = ""; break;
        }

        return format_to(ctx.begin(), "{}", cond_str);
    }
};

template <>
struct formatter<Gba::ShiftType> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Gba::ShiftType &type, FormatContext &ctx) {
        std::string type_str;

        switch (type) {
        case Gba::ShiftType::LSL: type_str = "LSL"; break;
        case Gba::ShiftType::LSR: type_str = "LSR"; break;
        case Gba::ShiftType::ASR: type_str = "ASR"; break;
        case Gba::ShiftType::ROR: type_str = "ROR"; break;
        case Gba::ShiftType::RRX: type_str = "RRX"; break;
        default:                  type_str = ""; break;
        }

        return format_to(ctx.begin(), "{}", type_str);
    }
};

} // End namespace fmt
