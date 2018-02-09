// This file is a part of Chroma.
// Copyright (C) 2018 Matthew Murray
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
#include <stdexcept>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include "gba/cpu/Disassembler.h"
#include "gba/cpu/Cpu.h"
#include "gba/cpu/Instruction.h"

namespace Gba {

Disassembler::Disassembler(const Memory& _mem, const Cpu& _cpu, LogLevel level)
        : mem(_mem)
        , cpu(_cpu)
        , thumb_instructions(Instruction<Thumb>::GetInstructionTable<Disassembler>())
        , arm_instructions(Instruction<Arm>::GetInstructionTable<Disassembler>())
        , log_level(level) {
    // Leave log_stream unopened if logging disabled.
    if (level != LogLevel::None) {
        log_stream = std::ofstream("log.txt");

        if (!log_stream) {
            throw std::runtime_error("Error when attempting to open ./log.txt for writing.");
        }
    }
}

// Needed to declare std::vector with forward-declared type in the header file.
Disassembler::~Disassembler() = default;

void Disassembler::DisassembleThumb(Thumb opcode, const std::array<u32, 16>& regs, u32 cpsr) {
    if (log_level == LogLevel::None) {
        return;
    }

    for (const auto& instr : thumb_instructions) {
        if (instr.Match(opcode)) {
            fmt::print(log_stream, "0x{:0>8X}, T: {}\n", cpu.GetPc(), instr.disasm_func(*this, opcode));
            break;
        }
    }

    if (log_level == LogLevel::Registers) {
        LogRegisters(regs, cpsr);
    }
}

void Disassembler::DisassembleArm(Arm opcode, const std::array<u32, 16>& regs, u32 cpsr) {
    if (log_level == LogLevel::None) {
        return;
    }

    for (const auto& instr : arm_instructions) {
        if (instr.Match(opcode)) {
            fmt::print(log_stream, "0x{:0>8X}, A: {}\n", cpu.GetPc(), instr.disasm_func(*this, opcode));
            break;
        }
    }

    if (log_level == LogLevel::Registers) {
        LogRegisters(regs, cpsr);
    }
}

void Disassembler::LogRegisters(const std::array<u32, 16>& regs, u32 cpsr) {
    fmt::MemoryWriter regs_str;
    for (int i = 0; i < 13; ++i) {
        regs_str.write("R{:X}=0x{:0>8X}, ", i, regs[i]);
        if (i == 4 || i == 9) {
            regs_str.write("\n");
        }
    }

    regs_str.write("SP=0x{:0>8X}, ", regs[sp]);
    regs_str.write("LR=0x{:0>8X}, ", regs[lr]);
    regs_str.write("{}", (cpsr & 0x8000'0000) ? "N" : "");
    regs_str.write("{}", (cpsr & 0x4000'0000) ? "Z" : "");
    regs_str.write("{}", (cpsr & 0x2000'0000) ? "C" : "");
    regs_str.write("{}\n\n", (cpsr & 0x1000'0000) ? "V" : "");

    fmt::print(log_stream, regs_str.str());
}

void Disassembler::LogHalt() {
    fmt::print(log_stream, "Halted for {} cycles\n", halt_cycles);
    halt_cycles = 0;
}

std::string Disassembler::RegStr(Reg r) {
    switch (r) {
    case sp:
        return "SP";
    case lr:
        return "LR";
    case pc:
        return "PC";
    default:
        return fmt::format("R{}", r);
    }
}

std::string Disassembler::ShiftStr(ImmediateShift shift) {
    if (shift.imm == 0) {
        return "";
    } else {
        return fmt::format(", {} #0x{:X}", shift.type, shift.imm);
    }
}

std::string Disassembler::ListStr(u32 reg_list) {
    const std::bitset<16> rlist{reg_list};
    Reg highest_reg = HighestSetBit(reg_list);
    fmt::MemoryWriter list_str;

    list_str << "{";

    for (Reg i = 0; i < 16; ++i) {
        if (rlist[i]) {
            if (i == highest_reg) {
                list_str << RegStr(i);
            } else {
                list_str << RegStr(i) << ", ";
            }
        }
    }

    list_str << "}";

    return list_str.str();
}

std::string Disassembler::AddrOffset(bool pre_indexed, bool add, bool wb, u32 imm) {
    if (pre_indexed) {
        if (imm == 0 && !wb) {
            return "]";
        } else {
            return fmt::format(", #{}0x{:X}]{}", (add) ? "+" : "-", imm, (wb) ? "!" : "");
        }
    } else {
        return fmt::format("], #{}0x{:X}", (add) ? "+" : "-", imm);
    }
}

std::string Disassembler::StatusReg(bool spsr, u32 mask) {
    fmt::MemoryWriter psr;

    if (spsr) {
        psr << "SPSR_";
    } else {
        psr << "CPSR_";
    }

    if (mask & 0x1) {
        psr << "c";
    } else if (mask & 0x8){
        psr << "f";
    }

    return psr.str();
}

void format_arg(fmt::BasicFormatter<char> &f, const char *&, const Condition &cond) {
    std::string cond_str;

    switch (cond) {
    case Condition::Equal:         cond_str = "EQ"; break;
    case Condition::NotEqual:      cond_str = "NE"; break;
    case Condition::CarrySet:      cond_str = "CS"; break;
    case Condition::CarryClear:    cond_str = "CC"; break;
    case Condition::Minus:         cond_str = "MI"; break;
    case Condition::Plus:          cond_str = "PL"; break;
    case Condition::OverflowSet:   cond_str = "VS"; break;
    case Condition::OverflowClear: cond_str = "VC"; break;
    case Condition::Higher:        cond_str = "HI"; break;
    case Condition::LowerSame:     cond_str = "LS"; break;
    case Condition::GreaterEqual:  cond_str = "GE"; break;
    case Condition::LessThan:      cond_str = "LT"; break;
    case Condition::GreaterThan:   cond_str = "GT"; break;
    case Condition::LessEqual:     cond_str = "LE"; break;
    default:                       cond_str = ""; break;
    }

    f.writer().write(cond_str);
}

void format_arg(fmt::BasicFormatter<char> &f, const char *&, const ShiftType &type) {
    std::string type_str;

    switch (type) {
    case ShiftType::LSL: type_str = "LSL"; break;
    case ShiftType::LSR: type_str = "LSR"; break;
    case ShiftType::ASR: type_str = "ASR"; break;
    case ShiftType::ROR: type_str = "ROR"; break;
    case ShiftType::RRX: type_str = "RRX"; break;
    default:             type_str = ""; break;
    }

    f.writer().write(type_str);
}

} // End namespace Gba
