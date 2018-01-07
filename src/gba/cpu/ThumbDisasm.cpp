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
#include <fmt/format.h>

#include "gba/cpu/Disassembler.h"
#include "gba/cpu/Cpu.h"
#include "gba/memory/Memory.h"

namespace Gba {

std::string Disassembler::AluImm(const char* name, Reg d, u32 imm) {
    return fmt::format("{} {}, #0x{:X}", name, RegStr(d), imm);
}

std::string Disassembler::AluRegImm(const char* name, u32 imm, Reg n, Reg d) {
    return fmt::format("{} {}, {}, #0x{:X}", name, RegStr(d), RegStr(n), imm);
}

std::string Disassembler::AluReg(const char* name, Reg m, Reg d) {
    return fmt::format("{} {}, {}", name, RegStr(d), RegStr(m));
}

std::string Disassembler::AluRegReg(const char* name, Reg m, Reg n, Reg d) {
    return fmt::format("{} {}, {}", name, RegStr(d), RegStr(n), RegStr(m));
}

std::string Disassembler::LoadImm(const char* name, u32 imm, Reg n, Reg t) {
    return fmt::format("{} {}, [{}{}", name, RegStr(t), RegStr(n), AddrOffset(true, true, false, imm));
}

std::string Disassembler::LoadReg(const char* name, Reg m, Reg n, Reg t) {
    return fmt::format("{} {}, [{}, {}]", name, RegStr(t), RegStr(n), RegStr(m));
}

// Arithmetic Operators
std::string Disassembler::Thumb_AdcReg(Reg m, Reg d) {
    return AluReg("ADCS", m, d);
}

std::string Disassembler::Thumb_AddImmT1(u32 imm, Reg n, Reg d) {
    return AluRegImm("ADDS", imm, n, d);
}

std::string Disassembler::Thumb_AddImmT2(Reg d, u32 imm) {
    return AluImm("ADDS", d, imm);
}

std::string Disassembler::Thumb_AddRegT1(Reg m, Reg n, Reg d) {
    return AluRegReg("ADDS", m, n, d);
}

std::string Disassembler::Thumb_AddRegT2(Reg d1, Reg m, Reg d2) {
    Reg d = (d1 << 3) | d2;
    return AluReg("ADD", m, d);
}

std::string Disassembler::Thumb_AddSpImmT1(Reg d, u32 imm) {
    imm <<= 2;
    return AluRegImm("ADD", imm, sp, d);
}

std::string Disassembler::Thumb_AddSpImmT2(u32 imm) {
    imm <<= 2;
    return AluRegImm("ADD", imm, sp, sp);
}

std::string Disassembler::Thumb_AddPcImm(Reg d, u32 imm) {
    imm <<= 2;
    return AluRegImm("ADD", imm, pc, d);
}

std::string Disassembler::Thumb_CmnReg(Reg m, Reg n) {
    return AluReg("CMN", m, n);
}

std::string Disassembler::Thumb_CmpImm(Reg n, u32 imm) {
    return AluImm("CMP", n, imm);
}

std::string Disassembler::Thumb_CmpRegT1(Reg m, Reg n) {
    return AluReg("CMP", m, n);
}

std::string Disassembler::Thumb_CmpRegT2(Reg n1, Reg m, Reg n2) {
    Reg n = (n1 << 3) | n2;
    return AluReg("CMP", m, n);
}

std::string Disassembler::Thumb_MulReg(Reg n, Reg d) {
    return AluReg("MULS", n, d);
}

std::string Disassembler::Thumb_RsbImm(Reg n, Reg d) {
    // The immediate is always 0 for this instruction.
    return AluRegImm("RSBS", 0, n, d);
}

std::string Disassembler::Thumb_SbcReg(Reg m, Reg d) {
    return AluReg("SBCS", m, d);
}

std::string Disassembler::Thumb_SubImmT1(u32 imm, Reg n, Reg d) {
    return AluRegImm("SUBS", imm, n, d);
}

std::string Disassembler::Thumb_SubImmT2(Reg d, u32 imm) {
    return AluImm("SUBS", d, imm);
}

std::string Disassembler::Thumb_SubReg(Reg m, Reg n, Reg d) {
    return AluRegReg("SUBS", m, n, d);
}

std::string Disassembler::Thumb_SubSpImm(u32 imm) {
    imm <<= 2;
    return AluRegImm("SUB", imm, sp, sp);
}

// Logical Operators
std::string Disassembler::Thumb_AndReg(Reg m, Reg d) {
    return AluReg("ANDS", m, d);
}

std::string Disassembler::Thumb_BicReg(Reg m, Reg d) {
    return AluReg("BICS", m, d);
}

std::string Disassembler::Thumb_EorReg(Reg m, Reg d) {
    return AluReg("EORS", m, d);
}

std::string Disassembler::Thumb_OrrReg(Reg m, Reg d) {
    return AluReg("ORRS", m, d);
}

std::string Disassembler::Thumb_TstReg(Reg m, Reg n) {
    return AluReg("TST", m, n);
}

// Shifts
std::string Disassembler::Thumb_AsrImm(u32 imm, Reg m, Reg d) {
    ImmediateShift shift = Cpu::DecodeImmShift(ShiftType::ASR, imm);
    return AluRegImm("ASRS", shift.imm, m, d);
}

std::string Disassembler::Thumb_AsrReg(Reg m, Reg d) {
    return AluReg("ASRS", m, d);
}

std::string Disassembler::Thumb_LslImm(u32 imm, Reg m, Reg d) {
    // No need to DecodeImmShift for LSL.
    return AluRegImm("LSLS", imm, m, d);
}

std::string Disassembler::Thumb_LslReg(Reg m, Reg d) {
    return AluReg("LSLS", m, d);
}

std::string Disassembler::Thumb_LsrImm(u32 imm, Reg m, Reg d) {
    ImmediateShift shift = Cpu::DecodeImmShift(ShiftType::LSR, imm);
    return AluRegImm("LSRS", shift.imm, m, d);
}

std::string Disassembler::Thumb_LsrReg(Reg m, Reg d) {
    return AluReg("LSRS", m, d);
}

std::string Disassembler::Thumb_RorReg(Reg m, Reg d) {
    return AluReg("RORS", m, d);
}

// Branches
std::string Disassembler::Thumb_BT1(Condition cond, u32 imm8) {
    s32 signed_imm32 = SignExtend(imm8 << 1, 9);
    return BranchImm("B", cond, signed_imm32);
}

std::string Disassembler::Thumb_BT2(u32 imm11) {
    s32 signed_imm32 = SignExtend(imm11 << 1, 12);
    return BranchImm("B", Condition::Always, signed_imm32);
}

std::string Disassembler::Thumb_BlH1(u32 imm11) {
    s32 signed_imm32 = SignExtend(imm11 << 12, 23);

    u16 next_instr = mem.ReadMem<u16>(cpu->GetPc() - 2);
    if ((next_instr & 0xF800) == 0xF800) {
        // If the next instruction is BlH2, disassemble as full BL.
        u32 imm_lo = (next_instr & ~0xF800) << 1;
        return BranchImm("BL", Condition::Always, signed_imm32 + imm_lo);
    } else {
        // If the next instruction is not BlH2, disassemble as an independent BLH1.
        return fmt::format("BLH1 #{:0>8X}", signed_imm32);
    }
}

std::string Disassembler::Thumb_BlH2(u32 imm11) {
    u16 prev_instr = mem.ReadMem<u16>(cpu->GetPc() - 6);
    if ((prev_instr & 0xF800) != 0xF000) {
        // If the previous instruction was not BlH1, disassemble as an independent BLH2.
        return fmt::format("BLH2 #{:0>8X}", imm11 << 1);
    } else {
        // If the previous instruction was BlH1, don't disassemble.
        return "";
    }
}

std::string Disassembler::Thumb_Bx(Reg m) {
    return fmt::format("BX {}", RegStr(m));
}

// Moves
std::string Disassembler::Thumb_MovImm(Reg d, u32 imm) {
    return AluImm("MOVS", d, imm);
}

std::string Disassembler::Thumb_MovRegT1(Reg d1, Reg m, Reg d2) {
    Reg d = (d1 << 3) | d2;
    return AluReg("MOV", m, d);
}

std::string Disassembler::Thumb_MovRegT2(Reg m, Reg d) {
    return AluReg("MOVS", m, d);
}

std::string Disassembler::Thumb_MvnReg(Reg m, Reg d) {
    return AluReg("MVNS", m, d);
}

// Loads
std::string Disassembler::Thumb_Ldm(Reg n, u32 reg_list) {
    const std::bitset<8> rlist{reg_list};
    return fmt::format("LDM {}{}, {}", RegStr(n), (rlist[n]) ? "" : "!", ListStr(reg_list));
}

std::string Disassembler::Thumb_LdrImm(u32 imm, Reg n, Reg t) {
    imm <<= 2;
    return LoadImm("LDR", imm, n, t);
}

std::string Disassembler::Thumb_LdrSpImm(Reg t, u32 imm) {
    imm <<= 2;
    return LoadImm("LDR", imm, sp, t);
}

std::string Disassembler::Thumb_LdrPcImm(Reg t, u32 imm) {
    imm <<= 2;
    return LoadImm("LDR", imm, pc, t);
}

std::string Disassembler::Thumb_LdrReg(Reg m, Reg n, Reg t) {
    return LoadReg("LDR", m, n, t);
}

std::string Disassembler::Thumb_LdrbImm(u32 imm, Reg n, Reg t) {
    return LoadImm("LDRB", imm, n, t);
}

std::string Disassembler::Thumb_LdrbReg(Reg m, Reg n, Reg t) {
    return LoadReg("LDRB", m, n, t);
}

std::string Disassembler::Thumb_LdrhImm(u32 imm, Reg n, Reg t) {
    imm <<= 1;
    return LoadImm("LDRH", imm, n, t);
}

std::string Disassembler::Thumb_LdrhReg(Reg m, Reg n, Reg t) {
    return LoadReg("LDRH", m, n, t);
}

std::string Disassembler::Thumb_LdrsbReg(Reg m, Reg n, Reg t) {
    return LoadReg("LDRSB", m, n, t);
}

std::string Disassembler::Thumb_LdrshReg(Reg m, Reg n, Reg t) {
    return LoadReg("LDRSH", m, n, t);
}

std::string Disassembler::Thumb_Pop(bool p, u32 reg_list) {
    if (p) {
        reg_list |= 1 << pc;
    }
    return fmt::format("POP {}", ListStr(reg_list));
}

// Stores
std::string Disassembler::Thumb_Push(bool m, u32 reg_list) {
    if (m) {
        reg_list |= 1 << lr;
    }
    return fmt::format("PUSH {}", ListStr(reg_list));
}

std::string Disassembler::Thumb_Stm(Reg n, u32 reg_list) {
    return fmt::format("STM {}!, {}", RegStr(n), ListStr(reg_list));
}

std::string Disassembler::Thumb_StrImm(u32 imm, Reg n, Reg t) {
    imm <<= 2;
    return LoadImm("STR", imm, n, t);
}

std::string Disassembler::Thumb_StrSpImm(Reg t, u32 imm) {
    imm <<= 2;
    return LoadImm("STR", imm, sp, t);
}

std::string Disassembler::Thumb_StrReg(Reg m, Reg n, Reg t) {
    return LoadReg("STR", m, n, t);
}

std::string Disassembler::Thumb_StrbImm(u32 imm, Reg n, Reg t) {
    return LoadImm("STRB", imm, n, t);
}

std::string Disassembler::Thumb_StrbReg(Reg m, Reg n, Reg t) {
    return LoadReg("STRB", m, n, t);
}

std::string Disassembler::Thumb_StrhImm(u32 imm, Reg n, Reg t) {
    imm <<= 1;
    return LoadImm("STRH", imm, n, t);
}

std::string Disassembler::Thumb_StrhReg(Reg m, Reg n, Reg t) {
    return LoadReg("STRH", m, n, t);
}

// Misc
std::string Disassembler::Thumb_Swi(u32 imm) {
    return fmt::format("SWI #0x{:X}", imm);
}

std::string Disassembler::Thumb_Undefined(u16 opcode) {
    return fmt::format("Undefined 0x{:0>4X}", opcode);
}

} // End namespace Gba
