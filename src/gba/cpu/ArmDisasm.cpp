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

#include <fmt/format.h>

#include "gba/cpu/Disassembler.h"
#include "gba/cpu/Cpu.h"

namespace Gba {

std::string Disassembler::AluImm(const char* name, Condition cond, bool sf, Reg n, Reg d, u32 imm) {
    imm = Cpu::ArmExpandImmediate(imm);
    return fmt::format("{}{}{} {}, {}, #0x{:X}", name, Flags(sf), cond, RegStr(d), RegStr(n), imm);
}

std::string Disassembler::AluReg(const char* name, Condition cond, bool sf, Reg n, Reg d, u32 imm,
                                 ShiftType type, Reg m) {
    ImmediateShift shift = Cpu::DecodeImmShift(type, imm);
    return fmt::format("{}{}{} {}, {}, {}{}", name, Flags(sf), cond, RegStr(d), RegStr(n), RegStr(m), ShiftStr(shift));
}

std::string Disassembler::AluRegShifted(const char* name, Condition cond, bool sf, Reg n, Reg d, Reg s,
                                        ShiftType type, Reg m) {
    return fmt::format("{}{}{} {}, {}, {}, {} {}", name, Flags(sf), cond, RegStr(d), RegStr(n), RegStr(m),
                       type, RegStr(s));
}

std::string Disassembler::FlagsImm(const char* name, Condition cond, bool sf, Reg n, u32 imm) {
    imm = Cpu::ArmExpandImmediate(imm);
    return fmt::format("{}{}{} {}, #0x{:X}", name, Flags(sf), cond, RegStr(n), imm);
}

std::string Disassembler::FlagsReg(const char* name, Condition cond, bool sf, Reg n, u32 imm, ShiftType type, Reg m) {
    ImmediateShift shift = Cpu::DecodeImmShift(type, imm);
    return fmt::format("{}{}{} {}, {}{}", name, Flags(sf), cond, RegStr(n), RegStr(m), ShiftStr(shift));
}

std::string Disassembler::FlagsRegShifted(const char* name, Condition cond, bool sf, Reg n, Reg s, ShiftType type,
                                          Reg m) {
    return fmt::format("{}{}{} {}, {}, {} {}", name, Flags(sf), cond, RegStr(n), RegStr(m), type, RegStr(s));
}

std::string Disassembler::MultiplyLong(const char* name, Condition cond, bool sf, Reg h, Reg l, Reg m, Reg n) {
    return fmt::format("{}{}{} {}, {}, {}, {}", name, Flags(sf), cond, RegStr(l), RegStr(h), RegStr(n), RegStr(m));
}

std::string Disassembler::ShiftImm(ShiftType type, Condition cond, bool sf, Reg d, u32 imm, Reg m) {
    ImmediateShift shift = Cpu::DecodeImmShift(type, imm);
    return fmt::format("{}{}{} {}, {}, #0x{:X}", shift.type, Flags(sf), cond, RegStr(d), RegStr(m), shift.imm);
}

std::string Disassembler::ShiftReg(const char* name, Condition cond, bool sf, Reg d, Reg m, Reg n) {
    return fmt::format("{}{}{} {}, {}, {}", name, Flags(sf), cond, RegStr(d), RegStr(n), RegStr(m));
}

std::string Disassembler::BranchImm(const char* name, Condition cond, s32 signed_imm32) {
    if (signed_imm32 < 0) {
        return fmt::format("{}{} PC -0x{:X}", name, cond, (~signed_imm32 + 1));
    } else {
        return fmt::format("{}{} PC +0x{:X}", name, cond, signed_imm32);
    }
}

std::string Disassembler::LoadMultiple(const char* name, Condition cond, bool pre_indexed, bool exception_return,
                                       bool wb, Reg n, u32 reg_list) {
    return fmt::format("{}{}{} {}{}, {}{}", name, (pre_indexed) ? "B" : "A", cond, RegStr(n),
                       (wb) ? "!" : "", ListStr(reg_list), (exception_return) ? "^" : "");
}

std::string Disassembler::LoadImm(const char* name, Condition cond, bool pre_indexed, bool add, bool wb,
                                  Reg n, Reg t, u32 imm) {
    return fmt::format("{}{} {}, [{}{}", name, cond, RegStr(t), RegStr(n), AddrOffset(pre_indexed, add, wb, imm));
}

std::string Disassembler::LoadReg(const char* name, Condition cond, bool pre_indexed, bool add, bool wb,
                                  Reg n, Reg t, u32 imm, ShiftType type, Reg m) {
    ImmediateShift shift = Cpu::DecodeImmShift(type, imm);

    if (pre_indexed) {
        return fmt::format("{}{} {}, [{}, {}{}{}]{}", name, cond, RegStr(t), RegStr(n), (add) ? "+" : "-",
                           RegStr(m), ShiftStr(shift), (wb) ? "!" : "");
    } else {
        return fmt::format("{}{} {}, [{}], {}{}{}", name, cond, RegStr(t), RegStr(n), (add) ? "+" : "-",
                           RegStr(m), ShiftStr(shift));
    }
}

// Arithmetic Operators
std::string Disassembler::Arm_AdcImm(Condition cond, bool sf, Reg n, Reg d, u32 imm) {
    return AluImm("ADC", cond, sf, n, d, imm);
}
std::string Disassembler::Arm_AdcReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    return AluReg("ADC", cond, sf, n, d, imm, type, m);
}
std::string Disassembler::Arm_AdcRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    return AluRegShifted("ADC", cond, sf, n, d, s, type, m);
}

std::string Disassembler::Arm_AddImm(Condition cond, bool sf, Reg n, Reg d, u32 imm) {
    return AluImm("ADD", cond, sf, n, d, imm);
}
std::string Disassembler::Arm_AddReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    return AluReg("ADD", cond, sf, n, d, imm, type, m);
}
std::string Disassembler::Arm_AddRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    return AluRegShifted("ADD", cond, sf, n, d, s, type, m);
}

std::string Disassembler::Arm_CmnImm(Condition cond, Reg n, u32 imm) {
    return FlagsImm("CMN", cond, false, n, imm);
}
std::string Disassembler::Arm_CmnReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m) {
    return FlagsReg("CMN", cond, false, n, imm, type, m);
}
std::string Disassembler::Arm_CmnRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m) {
    return FlagsRegShifted("CMN", cond, false, n, s, type, m);
}

std::string Disassembler::Arm_CmpImm(Condition cond, Reg n, u32 imm) {
    return FlagsImm("CMP", cond, false, n, imm);
}
std::string Disassembler::Arm_CmpReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m) {
    return FlagsReg("CMP", cond, false, n, imm, type, m);
}
std::string Disassembler::Arm_CmpRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m) {
    return FlagsRegShifted("CMP", cond, false, n, s, type, m);
}

std::string Disassembler::Arm_MlaReg(Condition cond, bool sf, Reg d, Reg a, Reg m, Reg n) {
    return fmt::format("MLA{}{} {}, {}, {}, {}", Flags(sf), cond, RegStr(d), RegStr(n), RegStr(m), RegStr(a));
}

std::string Disassembler::Arm_MulReg(Condition cond, bool sf, Reg d, Reg m, Reg n) {
    return fmt::format("MUL{}{} {}, {}, {}", Flags(sf), cond, RegStr(d), RegStr(n), RegStr(m));
}

std::string Disassembler::Arm_RsbImm(Condition cond, bool sf, Reg n, Reg d, u32 imm) {
    return AluImm("RSB", cond, sf, n, d, imm);
}
std::string Disassembler::Arm_RsbReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    return AluReg("RSB", cond, sf, n, d, imm, type, m);
}
std::string Disassembler::Arm_RsbRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    return AluRegShifted("RSB", cond, sf, n, d, s, type, m);
}

std::string Disassembler::Arm_RscImm(Condition cond, bool sf, Reg n, Reg d, u32 imm) {
    return AluImm("RSC", cond, sf, n, d, imm);
}
std::string Disassembler::Arm_RscReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    return AluReg("RSC", cond, sf, n, d, imm, type, m);
}
std::string Disassembler::Arm_RscRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    return AluRegShifted("RSC", cond, sf, n, d, s, type, m);
}

std::string Disassembler::Arm_SbcImm(Condition cond, bool sf, Reg n, Reg d, u32 imm) {
    return AluImm("SBC", cond, sf, n, d, imm);
}
std::string Disassembler::Arm_SbcReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    return AluReg("SBC", cond, sf, n, d, imm, type, m);
}
std::string Disassembler::Arm_SbcRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    return AluRegShifted("SBC", cond, sf, n, d, s, type, m);
}

std::string Disassembler::Arm_SmlalReg(Condition cond, bool sf, Reg dh, Reg dl, Reg m, Reg n) {
    return MultiplyLong("SMLAL", cond, sf, dh, dl, m, n);
}
std::string Disassembler::Arm_SmullReg(Condition cond, bool sf, Reg dh, Reg dl, Reg m, Reg n) {
    return MultiplyLong("SMULL", cond, sf, dh, dl, m, n);
}

std::string Disassembler::Arm_SubImm(Condition cond, bool sf, Reg n, Reg d, u32 imm) {
    return AluImm("SUB", cond, sf, n, d, imm);
}
std::string Disassembler::Arm_SubReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    return AluReg("SUB", cond, sf, n, d, imm, type, m);
}
std::string Disassembler::Arm_SubRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    return AluRegShifted("SUB", cond, sf, n, d, s, type, m);
}

std::string Disassembler::Arm_UmlalReg(Condition cond, bool sf, Reg dh, Reg dl, Reg m, Reg n) {
    return MultiplyLong("UMLAL", cond, sf, dh, dl, m, n);
}
std::string Disassembler::Arm_UmullReg(Condition cond, bool sf, Reg dh, Reg dl, Reg m, Reg n) {
    return MultiplyLong("UMULL", cond, sf, dh, dl, m, n);
}

// Logical Operators
std::string Disassembler::Arm_AndImm(Condition cond, bool sf, Reg n, Reg d, u32 imm) {
    return AluImm("AND", cond, sf, n, d, imm);
}
std::string Disassembler::Arm_AndReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    return AluReg("AND", cond, sf, n, d, imm, type, m);
}
std::string Disassembler::Arm_AndRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    return AluRegShifted("AND", cond, sf, n, d, s, type, m);
}

std::string Disassembler::Arm_BicImm(Condition cond, bool sf, Reg n, Reg d, u32 imm) {
    return AluImm("BIC", cond, sf, n, d, imm);
}
std::string Disassembler::Arm_BicReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    return AluReg("BIC", cond, sf, n, d, imm, type, m);
}
std::string Disassembler::Arm_BicRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    return AluRegShifted("BIC", cond, sf, n, d, s, type, m);
}

std::string Disassembler::Arm_EorImm(Condition cond, bool sf, Reg n, Reg d, u32 imm) {
    return AluImm("EOR", cond, sf, n, d, imm);
}
std::string Disassembler::Arm_EorReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    return AluReg("EOR", cond, sf, n, d, imm, type, m);
}
std::string Disassembler::Arm_EorRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    return AluRegShifted("EOR", cond, sf, n, d, s, type, m);
}

std::string Disassembler::Arm_OrrImm(Condition cond, bool sf, Reg n, Reg d, u32 imm) {
    return AluImm("ORR", cond, sf, n, d, imm);
}
std::string Disassembler::Arm_OrrReg(Condition cond, bool sf, Reg n, Reg d, u32 imm, ShiftType type, Reg m) {
    return AluReg("ORR", cond, sf, n, d, imm, type, m);
}
std::string Disassembler::Arm_OrrRegShifted(Condition cond, bool sf, Reg n, Reg d, Reg s, ShiftType type, Reg m) {
    return AluRegShifted("ORR", cond, sf, n, d, s, type, m);
}

std::string Disassembler::Arm_TeqImm(Condition cond, Reg n, u32 imm) {
    return FlagsImm("TEQ", cond, false, n, imm);
}
std::string Disassembler::Arm_TeqReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m) {
    return FlagsReg("TEQ", cond, false, n, imm, type, m);
}
std::string Disassembler::Arm_TeqRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m) {
    return FlagsRegShifted("TEQ", cond, false, n, s, type, m);
}

std::string Disassembler::Arm_TstImm(Condition cond, Reg n, u32 imm) {
    return FlagsImm("TST", cond, false, n, imm);
}
std::string Disassembler::Arm_TstReg(Condition cond, Reg n, u32 imm, ShiftType type, Reg m) {
    return FlagsReg("TST", cond, false, n, imm, type, m);
}
std::string Disassembler::Arm_TstRegShifted(Condition cond, Reg n, Reg s, ShiftType type, Reg m) {
    return FlagsRegShifted("TST", cond, false, n, s, type, m);
}

// Shifts
std::string Disassembler::Arm_AsrImm(Condition cond, bool sf, Reg d, u32 imm, Reg m) {
    return ShiftImm(ShiftType::ASR, cond, sf, d, imm, m);
}
std::string Disassembler::Arm_AsrReg(Condition cond, bool sf, Reg d, Reg m, Reg n) {
    return ShiftReg("ASR", cond, sf, d, m, n);
}

std::string Disassembler::Arm_LslImm(Condition cond, bool sf, Reg d, u32 imm, Reg m) {
    return ShiftImm(ShiftType::LSL, cond, sf, d, imm, m);
}
std::string Disassembler::Arm_LslReg(Condition cond, bool sf, Reg d, Reg m, Reg n) {
    return ShiftReg("LSL", cond, sf, d, m, n);
}

std::string Disassembler::Arm_LsrImm(Condition cond, bool sf, Reg d, u32 imm, Reg m) {
    return ShiftImm(ShiftType::LSR, cond, sf, d, imm, m);
}
std::string Disassembler::Arm_LsrReg(Condition cond, bool sf, Reg d, Reg m, Reg n) {
    return ShiftReg("LSR", cond, sf, d, m, n);
}

std::string Disassembler::Arm_RorImm(Condition cond, bool sf, Reg d, u32 imm, Reg m) {
    return ShiftImm(ShiftType::ROR, cond, sf, d, imm, m);
}
std::string Disassembler::Arm_RorReg(Condition cond, bool sf, Reg d, Reg m, Reg n) {
    return ShiftReg("ROR", cond, sf, d, m, n);
}

// Branches
std::string Disassembler::Arm_B(Condition cond, u32 imm24) {
    s32 signed_imm32 = SignExtend(imm24 << 2, 26);
    return BranchImm("B", cond, signed_imm32);
}
std::string Disassembler::Arm_Bl(Condition cond, u32 imm24) {
    s32 signed_imm32 = SignExtend(imm24 << 2, 26);
    return BranchImm("BL", cond, signed_imm32);
}
std::string Disassembler::Arm_Bx(Condition cond, Reg m) {
    return fmt::format("BX{} {}", cond, RegStr(m));
}

// Moves
std::string Disassembler::Arm_MovImm(Condition cond, bool sf, Reg d, u32 imm) {
    return FlagsImm("MOV", cond, sf, d, imm);
}
std::string Disassembler::Arm_MovReg(Condition cond, bool sf, Reg d, Reg m) {
    return FlagsReg("MOV", cond, sf, d, 0, ShiftType::LSL, m);
}

std::string Disassembler::Arm_MvnImm(Condition cond, bool sf, Reg d, u32 imm) {
    return FlagsImm("MVN", cond, sf, d, imm);
}
std::string Disassembler::Arm_MvnReg(Condition cond, bool sf, Reg d, u32 imm, ShiftType type, Reg m) {
    return FlagsReg("MVN", cond, sf, d, imm, type, m);
}
std::string Disassembler::Arm_MvnRegShifted(Condition cond, bool sf, Reg d, Reg s, ShiftType type, Reg m) {
    return FlagsRegShifted("MVN", cond, sf, d, s, type, m);
}

// Loads
std::string Disassembler::Arm_Ldmi(Condition cond, bool pre_indexed, bool exception_return, bool wb, Reg n, u32 reg_list) {
    return LoadMultiple("LDMI", cond, pre_indexed, exception_return, wb, n, reg_list);
}
std::string Disassembler::Arm_Ldmd(Condition cond, bool pre_indexed, bool exception_return, bool wb, Reg n, u32 reg_list) {
    return LoadMultiple("LDMD", cond, pre_indexed, exception_return, wb, n, reg_list);
}

std::string Disassembler::Arm_LdrImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm) {
    return LoadImm("LDR", cond, pre_indexed, add, wb, n, t, imm);
}
std::string Disassembler::Arm_LdrReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm,
                ShiftType type, Reg m) {
    return LoadReg("LDR", cond, pre_indexed, add, wb, n, t, imm, type, m);
}

std::string Disassembler::Arm_LdrbImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm) {
    return LoadImm("LDRB", cond, pre_indexed, add, wb, n, t, imm);
}
std::string Disassembler::Arm_LdrbReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm,
                ShiftType type, Reg m) {
    return LoadReg("LDRB", cond, pre_indexed, add, wb, n, t, imm, type, m);
}

std::string Disassembler::Arm_LdrhImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm_hi, u32 imm_lo) {
    u32 imm = (imm_hi << 4) | imm_lo;
    return LoadImm("LDRH", cond, pre_indexed, add, wb, n, t, imm);
}
std::string Disassembler::Arm_LdrhReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, Reg m) {
    return LoadReg("LDRH", cond, pre_indexed, add, wb, n, t, 0, ShiftType::LSL, m);
}

std::string Disassembler::Arm_LdrsbImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm_hi, u32 imm_lo) {
    u32 imm = (imm_hi << 4) | imm_lo;
    return LoadImm("LDRSB", cond, pre_indexed, add, wb, n, t, imm);
}
std::string Disassembler::Arm_LdrsbReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, Reg m) {
    return LoadReg("LDRSB", cond, pre_indexed, add, wb, n, t, 0, ShiftType::LSL, m);
}

std::string Disassembler::Arm_LdrshImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm_hi, u32 imm_lo) {
    u32 imm = (imm_hi << 4) | imm_lo;
    return LoadImm("LDRSH", cond, pre_indexed, add, wb, n, t, imm);
}
std::string Disassembler::Arm_LdrshReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, Reg m) {
    return LoadReg("LDRSH", cond, pre_indexed, add, wb, n, t, 0, ShiftType::LSL, m);
}

std::string Disassembler::Arm_PopA1(Condition cond, u32 reg_list) {
    return fmt::format("POP{} {}", cond, ListStr(reg_list));
}
std::string Disassembler::Arm_PopA2(Condition cond, Reg t) {
    return fmt::format("POP{} {}", cond, RegStr(t));
}

// Stores
std::string Disassembler::Arm_PushA1(Condition cond, u32 reg_list) {
    return fmt::format("PUSH{} {}", cond, ListStr(reg_list));
}
std::string Disassembler::Arm_PushA2(Condition cond, Reg t) {
    return fmt::format("PUSH{} {}", cond, RegStr(t));
}

std::string Disassembler::Arm_Stmi(Condition cond, bool pre_indexed, bool user_regs, bool wb, Reg n, u32 reg_list) {
    return LoadMultiple("STMI", cond, pre_indexed, user_regs, wb, n, reg_list);
}
std::string Disassembler::Arm_Stmd(Condition cond, bool pre_indexed, bool user_regs, bool wb, Reg n, u32 reg_list) {
    return LoadMultiple("STMD", cond, pre_indexed, user_regs, wb, n, reg_list);
}

std::string Disassembler::Arm_StrImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm) {
    return LoadImm("STR", cond, pre_indexed, add, wb, n, t, imm);
}
std::string Disassembler::Arm_StrReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm,
                ShiftType type, Reg m) {
    return LoadReg("STR", cond, pre_indexed, add, wb, n, t, imm, type, m);
}

std::string Disassembler::Arm_StrbImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm) {
    return LoadImm("STRB", cond, pre_indexed, add, wb, n, t, imm);
}
std::string Disassembler::Arm_StrbReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm,
                ShiftType type, Reg m) {
    return LoadReg("STRB", cond, pre_indexed, add, wb, n, t, imm, type, m);
}

std::string Disassembler::Arm_StrhImm(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, u32 imm_hi, u32 imm_lo) {
    u32 imm = (imm_hi << 4) | imm_lo;
    return LoadImm("STRH", cond, pre_indexed, add, wb, n, t, imm);
}
std::string Disassembler::Arm_StrhReg(Condition cond, bool pre_indexed, bool add, bool wb, Reg n, Reg t, Reg m) {
    return LoadReg("STRH", cond, pre_indexed, add, wb, n, t, 0, ShiftType::LSL, m);
}

std::string Disassembler::Arm_SwpReg(Condition cond, bool byte, Reg n, Reg t1, Reg t2) {
    return fmt::format("SWP{}{} {}, {}, [{}]", (byte) ? "B" : "", cond, RegStr(t1), RegStr(t2), RegStr(n));
}

// Misc
std::string Disassembler::Arm_Cdp(Condition cond, u32 opcode1, Reg cn, Reg cd, u32 coproc, u32 opcode2, Reg cm) {
    return fmt::format("CDP{} p{}, #{}, {}, {}, {}, #{}", cond, coproc, opcode1, cd, cn, cm, opcode2);
}
std::string Disassembler::Arm_Ldc(Condition cond, bool p, bool u, bool, bool w, Reg n, Reg cd, u32 coproc, u32 imm) {
    return fmt::format("LDC{} p{}, {}, [{}", cond, coproc, cd, RegStr(n), AddrOffset(p, u, w, imm << 2));
}
std::string Disassembler::Arm_Mcr(Condition cond, u32 opcode1, Reg cn, Reg t, u32 coproc, u32 opcode2, Reg cm) {
    return fmt::format("MCR{} p{}, #{}, {}, {}, {}, #{}", cond, coproc, opcode1, RegStr(t), cn, cm, opcode2);
}

std::string Disassembler::Arm_Mrs(Condition cond, bool read_spsr, Reg d) {
    return fmt::format("MRS{} {}, {}", cond, RegStr(d), (read_spsr) ? "SPSR" : "CPSR");
}

std::string Disassembler::Arm_MsrImm(Condition cond, bool write_spsr, u32 mask, u32 imm) {
    imm = Cpu::ArmExpandImmediate(imm);
    return fmt::format("MSR{} {}, #0x{:X}", cond, StatusReg(write_spsr, mask), imm);
}
std::string Disassembler::Arm_MsrReg(Condition cond, bool write_spsr, u32 mask, Reg n) {
    return fmt::format("MSR{} {}, {}", cond, StatusReg(write_spsr, mask), RegStr(n));
}

std::string Disassembler::Arm_Swi(Condition cond, u32 imm) {
    return fmt::format("SWI{} #0x{:X}", cond, imm);
}

std::string Disassembler::Arm_Undefined(u32 opcode) {
    return fmt::format("Undefined 0x{:0>8X}", opcode);
}

} // End namespace Gba
