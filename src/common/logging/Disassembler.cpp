// This file is a part of Chroma.
// Copyright (C) 2016 Matthew Murray
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

#include <iomanip>
#include <sstream>

#include "common/logging/Logging.h"
#include "core/memory/Memory.h"

namespace Log {

std::string NextByteAsStr(const Core::Memory& mem, const u16 pc) {
    std::ostringstream hex_str;
    hex_str << std::hex << std::uppercase;
    hex_str << "0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(pc+1));
    return hex_str.str();
}

std::string NextSignedByteAsStr(const Core::Memory& mem, const u16 pc) {
    std::ostringstream hex_str;
    hex_str << std::hex << std::uppercase;
    s8 sbyte = mem.ReadMem8(pc+1);
    if (sbyte < 0) {
        hex_str << "- 0x" << std::setfill('0') << std::setw(2) << (~sbyte)+1;
    } else {
        hex_str << "+ 0x" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(sbyte);
    }
    return hex_str.str();
}

std::string NextWordAsStr(const Core::Memory& mem, const u16 pc) {
    std::ostringstream hex_str;
    hex_str << std::hex << std::uppercase;
    hex_str << "0x";
    hex_str << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(pc+2));
    hex_str << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(mem.ReadMem8(pc+1));
    return hex_str.str();
}

void LoadString(std::ostringstream& instr, const std::string into, const std::string from) {
    instr << "LD " << into << ", " << from;
}

void LoadIncString(std::ostringstream& instr, const std::string into, const std::string from) {
    instr << "LDI " << into << ", " << from;
}

void LoadDecString(std::ostringstream& instr, const std::string into, const std::string from) {
    instr << "LDD " << into << ", " << from;
}

void LoadHighString(std::ostringstream& instr, const std::string into, const std::string from) {
    instr << "LDH " << into << ", " << from;
}

void PushString(std::ostringstream& instr, const std::string reg) {
    instr << "PUSH " << reg;
}

void PopString(std::ostringstream& instr, const std::string reg) {
    instr << "POP " << reg;
}

void AddString(std::ostringstream& instr, const std::string from) {
    instr << "ADD A, " << from;
}

void AddString(std::ostringstream& instr, const std::string into, const std::string from) {
    instr << "ADD " << into << ", " << from;
}

void AdcString(std::ostringstream& instr, const std::string from) {
    instr << "ADC A, " << from;
}

void SubString(std::ostringstream& instr, const std::string from) {
    instr << "SUB A, " << from;
}

void SbcString(std::ostringstream& instr, const std::string from) {
    instr << "SBC A, " << from;
}

void AndString(std::ostringstream& instr, const std::string with) {
    instr << "AND " << with;
}

void OrString(std::ostringstream& instr, const std::string with) {
    instr << "OR " << with;
}

void XorString(std::ostringstream& instr, const std::string with) {
    instr << "XOR " << with;
}

void CompareString(std::ostringstream& instr, const std::string with) {
    instr << "CP " << with;
}

void IncString(std::ostringstream& instr, const std::string reg) {
    instr << "INC " << reg;
}

void DecString(std::ostringstream& instr, const std::string reg) {
    instr << "DEC " << reg;
}

void JumpString(std::ostringstream& instr, const std::string addr) {
    instr << "JP " << addr;
}

void JumpString(std::ostringstream& instr, const std::string cond, const std::string addr) {
    instr << "JP " << cond << ", " << addr;
}

void RelJumpString(std::ostringstream& instr, const std::string addr) {
    instr << "JR " << addr;
}

void RelJumpString(std::ostringstream& instr, const std::string cond, const std::string addr) {
    instr << "JR " << cond << ", " << addr;
}

void CallString(std::ostringstream& instr, const std::string addr) {
    instr << "CALL " << addr;
}

void CallString(std::ostringstream& instr, const std::string cond, const std::string addr) {
    instr << "CALL " << cond << ", " << addr;
}

void ReturnInterruptString(std::ostringstream& instr, const std::string reti) {
    instr << "RET" << reti;
}

void ReturnCondString(std::ostringstream& instr, const std::string cond) {
    instr << "RET " << cond;
}

void RestartString(std::ostringstream& instr, const std::string addr) {
    instr << "RST " << addr;
}

void RotLeftString(std::ostringstream& instr, const std::string carry, const std::string reg) {
    instr << "RL" << carry << " " << reg;
}

void RotRightString(std::ostringstream& instr, const std::string carry, const std::string reg) {
    instr << "RR" << carry << " " << reg;
}

void ShiftLeftString(std::ostringstream& instr, const std::string reg) {
    instr << "SLA " << reg;
}

void ShiftRightString(std::ostringstream& instr, const std::string a_or_l, const std::string reg) {
    instr << "SR" << a_or_l << " " << reg;
}

void SwapString(std::ostringstream& instr, const std::string reg) {
    instr << "SWAP " << reg;
}

void TestBitString(std::ostringstream& instr, const std::string bit, const std::string reg) {
    instr << "BIT " << bit << ", " << reg;
}

void ResetBitString(std::ostringstream& instr, const std::string bit, const std::string reg) {
    instr << "RES " << bit << ", " << reg;
}

void SetBitString(std::ostringstream& instr, const std::string bit, const std::string reg) {
    instr << "SET " << bit << ", " << reg;
}

void UnknownOpcodeString(std::ostringstream& instr, const Core::Memory& mem, const u16 pc) {
    instr << "Unknown Opcode: ";
    instr << std::hex << std::setfill('0') << std::setw(2) << std::uppercase;
    instr << "0x" << static_cast<unsigned int>(mem.ReadMem8(pc));
}

std::string Logging::Disassemble(const Core::Memory& mem, const u16 pc) const {
    std::ostringstream instr_stream;

    switch (mem.ReadMem8(pc)) {
    // ******** 8-bit loads ********
    // LD R, n -- Load immediate value n into register R
    case 0x06:
        LoadString(instr_stream, "B", NextByteAsStr(mem, pc));
        break;
    case 0x0E:
        LoadString(instr_stream, "C", NextByteAsStr(mem, pc));
        break;
    case 0x16:
        LoadString(instr_stream, "D", NextByteAsStr(mem, pc));
        break;
    case 0x1E:
        LoadString(instr_stream, "E", NextByteAsStr(mem, pc));
        break;
    case 0x26:
        LoadString(instr_stream, "H", NextByteAsStr(mem, pc));
        break;
    case 0x2E:
        LoadString(instr_stream, "L", NextByteAsStr(mem, pc));
        break;
    case 0x3E:
        LoadString(instr_stream, "A", NextByteAsStr(mem, pc));
        break;
    // LD A, R2 -- Load value from R2 into A
    case 0x78:
        LoadString(instr_stream, "A", "B");
        break;
    case 0x79:
        LoadString(instr_stream, "A", "C");
        break;
    case 0x7A:
        LoadString(instr_stream, "A", "D");
        break;
    case 0x7B:
        LoadString(instr_stream, "A", "E");
        break;
    case 0x7C:
        LoadString(instr_stream, "A", "H");
        break;
    case 0x7D:
        LoadString(instr_stream, "A", "L");
        break;
    case 0x7E:
        LoadString(instr_stream, "A", "(HL)");
        break;
    case 0x7F:
        LoadString(instr_stream, "A", "A");
        break;
    // LD B, R2 -- Load value from R2 into B
    case 0x40:
        LoadString(instr_stream, "B", "B");
        break;
    case 0x41:
        LoadString(instr_stream, "B", "C");
        break;
    case 0x42:
        LoadString(instr_stream, "B", "D");
        break;
    case 0x43:
        LoadString(instr_stream, "B", "E");
        break;
    case 0x44:
        LoadString(instr_stream, "B", "H");
        break;
    case 0x45:
        LoadString(instr_stream, "B", "L");
        break;
    case 0x46:
        LoadString(instr_stream, "B", "(HL)");
        break;
    case 0x47:
        LoadString(instr_stream, "B", "A");
        break;
    // LD C, R2 -- Load value from R2 into C
    case 0x48:
        LoadString(instr_stream, "C", "B");
        break;
    case 0x49:
        LoadString(instr_stream, "C", "C");
        break;
    case 0x4A:
        LoadString(instr_stream, "C", "D");
        break;
    case 0x4B:
        LoadString(instr_stream, "C", "E");
        break;
    case 0x4C:
        LoadString(instr_stream, "C", "H");
        break;
    case 0x4D:
        LoadString(instr_stream, "C", "L");
        break;
    case 0x4E:
        LoadString(instr_stream, "C", "(HL)");
        break;
    case 0x4F:
        LoadString(instr_stream, "C", "A");
        break;
    // LD D, R2 -- Load value from R2 into D
    case 0x50:
        LoadString(instr_stream, "D", "B");
        break;
    case 0x51:
        LoadString(instr_stream, "D", "C");
        break;
    case 0x52:
        LoadString(instr_stream, "D", "D");
        break;
    case 0x53:
        LoadString(instr_stream, "D", "E");
        break;
    case 0x54:
        LoadString(instr_stream, "D", "H");
        break;
    case 0x55:
        LoadString(instr_stream, "D", "L");
        break;
    case 0x56:
        LoadString(instr_stream, "C", "(HL)");
        break;
    case 0x57:
        LoadString(instr_stream, "D", "A");
        break;
    // LD E, R2 -- Load value from R2 into E
    case 0x58:
        LoadString(instr_stream, "E", "B");
        break;
    case 0x59:
        LoadString(instr_stream, "E", "C");
        break;
    case 0x5A:
        LoadString(instr_stream, "E", "D");
        break;
    case 0x5B:
        LoadString(instr_stream, "E", "E");
        break;
    case 0x5C:
        LoadString(instr_stream, "E", "H");
        break;
    case 0x5D:
        LoadString(instr_stream, "E", "L");
        break;
    case 0x5E:
        LoadString(instr_stream, "E", "(HL)");
        break;
    case 0x5F:
        LoadString(instr_stream, "E", "A");
        break;
    // LD H, R2 -- Load value from R2 into H
    case 0x60:
        LoadString(instr_stream, "H", "B");
        break;
    case 0x61:
        LoadString(instr_stream, "H", "C");
        break;
    case 0x62:
        LoadString(instr_stream, "H", "D");
        break;
    case 0x63:
        LoadString(instr_stream, "H", "E");
        break;
    case 0x64:
        LoadString(instr_stream, "H", "H");
        break;
    case 0x65:
        LoadString(instr_stream, "H", "L");
        break;
    case 0x66:
        LoadString(instr_stream, "H", "(HL)");
        break;
    case 0x67:
        LoadString(instr_stream, "H", "A");
        break;
    // LD L, R2 -- Load value from R2 into L
    case 0x68:
        LoadString(instr_stream, "L", "B");
        break;
    case 0x69:
        LoadString(instr_stream, "L", "C");
        break;
    case 0x6A:
        LoadString(instr_stream, "L", "D");
        break;
    case 0x6B:
        LoadString(instr_stream, "L", "E");
        break;
    case 0x6C:
        LoadString(instr_stream, "L", "H");
        break;
    case 0x6D:
        LoadString(instr_stream, "L", "L");
        break;
    case 0x6E:
        LoadString(instr_stream, "L", "(HL)");
        break;
    case 0x6F:
        LoadString(instr_stream, "L", "A");
        break;
    // LD (HL), R2 -- Load value from R2 into memory at (HL)
    case 0x70:
        LoadString(instr_stream, "(HL)", "B");
        break;
    case 0x71:
        LoadString(instr_stream, "(HL)", "C");
        break;
    case 0x72:
        LoadString(instr_stream, "(HL)", "D");
        break;
    case 0x73:
        LoadString(instr_stream, "(HL)", "E");
        break;
    case 0x74:
        LoadString(instr_stream, "(HL)", "H");
        break;
    case 0x75:
        LoadString(instr_stream, "(HL)", "L");
        break;
    case 0x77:
        LoadString(instr_stream, "(HL)", "A");
        break;
    case 0x36:
        LoadString(instr_stream, "(HL)", NextByteAsStr(mem, pc));
        break;
    // LD A, (nn) -- Load value from memory at (nn) into A
    case 0x0A:
        LoadString(instr_stream, "A", "(BC)");
        break;
    case 0x1A:
        LoadString(instr_stream, "A", "(DE)");
        break;
    case 0xFA:
        LoadString(instr_stream, "A", "(" + NextWordAsStr(mem, pc) + ")");
        break;
    // LD (nn), A -- Load value from A into memory at (nn)
    case 0x02:
        LoadString(instr_stream, "(BC)", "A");
        break;
    case 0x12:
        LoadString(instr_stream, "(DE)", "A");
        break;
    case 0xEA:
        LoadString(instr_stream, "(" + NextWordAsStr(mem, pc) + ")", "A");
        break;
    // LD (C), A -- Load value from A into memory at (0xFF00 + C)
    case 0xE2:
        LoadString(instr_stream, "(0xFF00 + C)", "A");
        break;
    // LD A, (C) -- Load value from memory at (0xFF00 + C) into A
    case 0xF2:
        LoadString(instr_stream, "A", "(0xFF00 + C)");
        break;
    // LDI (HL), A -- Load value from A into memory at (HL), then increment HL
    case 0x22:
        LoadIncString(instr_stream, "(HL)", "A");
        break;
    // LDI A, (HL) -- Load value from memory at (HL) into A, then increment HL
    case 0x2A:
        LoadIncString(instr_stream, "A", "(HL)");
        break;
    // LDD (HL), A -- Load value from A into memory at (HL), then decrement HL
    case 0x32:
        LoadDecString(instr_stream, "(HL)", "A");
        break;
    // LDD A, (HL) -- Load value from memory at (HL) into A, then decrement HL
    case 0x3A:
        LoadDecString(instr_stream, "A", "(HL)");
        break;
    // LDH (n), A -- Load value from A into memory at (0xFF00+n), with n as immediate byte value
    case 0xE0:
        LoadHighString(instr_stream, "(0xFF00 + " + NextByteAsStr(mem, pc) + ")", "A");
        break;
    // LDH A, (n) -- Load value from memory at (0xFF00+n) into A, with n as immediate byte value 
    case 0xF0:
        LoadHighString(instr_stream, "A", "(0xFF00 + " + NextByteAsStr(mem, pc) + ")");
        break;

    // ******** 16-bit loads ********
    // LD R, nn -- Load 16-bit immediate value into 16-bit register R
    case 0x01:
        LoadString(instr_stream, "BC", NextWordAsStr(mem, pc));
        break;
    case 0x11:
        LoadString(instr_stream, "DE", NextWordAsStr(mem, pc));
        break;
    case 0x21:
        LoadString(instr_stream, "HL", NextWordAsStr(mem, pc));
        break;
    case 0x31:
        LoadString(instr_stream, "SP", NextWordAsStr(mem, pc));
        break;
    // LD SP, HL -- Load value from HL into SP
    case 0xF9:
        LoadString(instr_stream, "SP", "HL");
        break;
    // LD HL, SP+n -- Load value from SP + n into HL, with n as signed immediate byte value
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Set appropriately, with immediate as unsigned byte.
    //     C: Set appropriately, with immediate as unsigned byte.
    case 0xF8:
        LoadString(instr_stream, "HL", "SP" + NextSignedByteAsStr(mem, pc));
        break;
    // LD (nn), SP -- Load value from SP into memory at (nn)
    case 0x08:
        LoadString(instr_stream, "(" + NextWordAsStr(mem, pc) + ")", "SP");
        break;
    // PUSH R -- Push 16-bit register R onto the stack and decrement the stack pointer by 2
    case 0xC5:
        PushString(instr_stream, "BC");
        break;
    case 0xD5:
        PushString(instr_stream, "DE");
        break;
    case 0xE5:
        PushString(instr_stream, "HL");
        break;
    case 0xF5:
        PushString(instr_stream, "AF");
        break;
    // POP R -- Pop 2 bytes off the stack into 16-bit register R and increment the stack pointer by 2
    case 0xC1:
        PopString(instr_stream, "BC");
        break;
    case 0xD1:
        PopString(instr_stream, "DE");
        break;
    case 0xE1:
        PopString(instr_stream, "HL");
        break;
    case 0xF1:
        PopString(instr_stream, "AF");
        break;

    // ******** 8-bit arithmetic and logic ********
    // ADD A, R -- Add value in register R to A
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Set if carry from bit 7
    case 0x80:
        AddString(instr_stream, "B");
        break;
    case 0x81:
        AddString(instr_stream, "C");
        break;
    case 0x82:
        AddString(instr_stream, "D");
        break;
    case 0x83:
        AddString(instr_stream, "E");
        break;
    case 0x84:
        AddString(instr_stream, "H");
        break;
    case 0x85:
        AddString(instr_stream, "L");
        break;
    case 0x86:
        AddString(instr_stream, "(HL)");
        break;
    case 0x87:
        AddString(instr_stream, "A");
        break;
    // ADD A, n -- Add immediate value n to A
    // Flags: same as ADD A, R
    case 0xC6:
        AddString(instr_stream, NextByteAsStr(mem, pc));
        break;
    // ADC A, R -- Add value in register R + the carry flag to A
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Set if carry from bit 7
    case 0x88:
        AdcString(instr_stream, "B");
        break;
    case 0x89:
        AdcString(instr_stream, "C");
        break;
    case 0x8A:
        AdcString(instr_stream, "D");
        break;
    case 0x8B:
        AdcString(instr_stream, "E");
        break;
    case 0x8C:
        AdcString(instr_stream, "H");
        break;
    case 0x8D:
        AdcString(instr_stream, "L");
        break;
    case 0x8E:
        AdcString(instr_stream, "(HL)");
        break;
    case 0x8F:
        AdcString(instr_stream, "A");
        break;
    // ADC A, n -- Add immediate value n + the carry flag to A
    // Flags: same as ADC A, R
    case 0xCE:
        AdcString(instr_stream, NextByteAsStr(mem, pc));
        break;
    // SUB R -- Subtract the value in register R from  A
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0x90:
        SubString(instr_stream, "B");
        break;
    case 0x91:
        SubString(instr_stream, "C");
        break;
    case 0x92:
        SubString(instr_stream, "D");
        break;
    case 0x93:
        SubString(instr_stream, "E");
        break;
    case 0x94:
        SubString(instr_stream, "H");
        break;
    case 0x95:
        SubString(instr_stream, "L");
        break;
    case 0x96:
        SubString(instr_stream, "(HL)");
        break;
    case 0x97:
        SubString(instr_stream, "A");
        break;
    // SUB n -- Subtract immediate value n from  A
    // Flags: same as SUB R
    case 0xD6:
        SubString(instr_stream, NextByteAsStr(mem, pc));
        break;
    // SBC A, R -- Subtract the value in register R + carry flag from  A
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0x98:
        SbcString(instr_stream, "B");
        break;
    case 0x99:
        SbcString(instr_stream, "C");
        break;
    case 0x9A:
        SbcString(instr_stream, "D");
        break;
    case 0x9B:
        SbcString(instr_stream, "E");
        break;
    case 0x9C:
        SbcString(instr_stream, "H");
        break;
    case 0x9D:
        SbcString(instr_stream, "L");
        break;
    case 0x9E:
        SbcString(instr_stream, "(HL)");
        break;
    case 0x9F:
        SbcString(instr_stream, "A");
        break;
    // SBC A, n -- Subtract immediate value n + carry flag from  A
    // Flags: same as SBC A, R
    case 0xDE:
        SbcString(instr_stream, NextByteAsStr(mem, pc));
        break;
    // AND R -- Bitwise AND the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set
    //     C: Reset
    case 0xA0:
        AndString(instr_stream, "B");
        break;
    case 0xA1:
        AndString(instr_stream, "C");
        break;
    case 0xA2:
        AndString(instr_stream, "D");
        break;
    case 0xA3:
        AndString(instr_stream, "E");
        break;
    case 0xA4:
        AndString(instr_stream, "H");
        break;
    case 0xA5:
        AndString(instr_stream, "L");
        break;
    case 0xA6:
        AndString(instr_stream, "(HL)");
        break;
    case 0xA7:
        AndString(instr_stream, "A");
        break;
    // AND n -- Bitwise AND the immediate value with A. 
    // Flags: same as AND R
    case 0xE6:
        AndString(instr_stream, NextByteAsStr(mem, pc));
        break;
    // OR R -- Bitwise OR the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Reset
    //     C: Reset
    case 0xB0:
        OrString(instr_stream, "B");
        break;
    case 0xB1:
        OrString(instr_stream, "C");
        break;
    case 0xB2:
        OrString(instr_stream, "D");
        break;
    case 0xB3:
        OrString(instr_stream, "E");
        break;
    case 0xB4:
        OrString(instr_stream, "H");
        break;
    case 0xB5:
        OrString(instr_stream, "L");
        break;
    case 0xB6:
        OrString(instr_stream, "(HL)");
        break;
    case 0xB7:
        OrString(instr_stream, "A");
        break;
    // OR n -- Bitwise OR the immediate value with A. 
    // Flags: same as OR R
    case 0xF6:
        OrString(instr_stream, NextByteAsStr(mem, pc));
        break;
    // XOR R -- Bitwise XOR the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Reset
    //     C: Reset
    case 0xA8:
        XorString(instr_stream, "B");
        break;
    case 0xA9:
        XorString(instr_stream, "C");
        break;
    case 0xAA:
        XorString(instr_stream, "D");
        break;
    case 0xAB:
        XorString(instr_stream, "E");
        break;
    case 0xAC:
        XorString(instr_stream, "H");
        break;
    case 0xAD:
        XorString(instr_stream, "L");
        break;
    case 0xAE:
        XorString(instr_stream, "(HL)");
        break;
    case 0xAF:
        XorString(instr_stream, "A");
        break;
    // XOR n -- Bitwise XOR the immediate value with A. 
    // Flags: same as XOR R
    case 0xEE:
        XorString(instr_stream, NextByteAsStr(mem, pc));
        break;
    // CP R -- Compare A with the value in register R. This performs a subtraction but does not modify A.
    // Flags:
    //     Z: Set if result is zero, i.e. A is equal to R
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0xB8:
        CompareString(instr_stream, "B");
        break;
    case 0xB9:
        CompareString(instr_stream, "C");
        break;
    case 0xBA:
        CompareString(instr_stream, "D");
        break;
    case 0xBB:
        CompareString(instr_stream, "E");
        break;
    case 0xBC:
        CompareString(instr_stream, "H");
        break;
    case 0xBD:
        CompareString(instr_stream, "L");
        break;
    case 0xBE:
        CompareString(instr_stream, "(HL)");
        break;
    case 0xBF:
        CompareString(instr_stream, "A");
        break;
    // CP n -- Compare A with the immediate value. This performs a subtraction but does not modify A.
    // Flags: same as CP R
    case 0xFE:
        CompareString(instr_stream, NextByteAsStr(mem, pc));
        break;
    // INC R -- Increment the value in register R.
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Unchanged
    case 0x04:
        IncString(instr_stream, "B");
        break;
    case 0x0C:
        IncString(instr_stream, "C");
        break;
    case 0x14:
        IncString(instr_stream, "D");
        break;
    case 0x1C:
        IncString(instr_stream, "E");
        break;
    case 0x24:
        IncString(instr_stream, "H");
        break;
    case 0x2C:
        IncString(instr_stream, "L");
        break;
    case 0x34:
        IncString(instr_stream, "(HL)");
        break;
    case 0x3C:
        IncString(instr_stream, "A");
        break;
    // DEC R -- Decrement the value in register R.
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Unchanged
    case 0x05:
        DecString(instr_stream, "B");
        break;
    case 0x0D:
        DecString(instr_stream, "C");
        break;
    case 0x15:
        DecString(instr_stream, "D");
        break;
    case 0x1D:
        DecString(instr_stream, "E");
        break;
    case 0x25:
        DecString(instr_stream, "H");
        break;
    case 0x2D:
        DecString(instr_stream, "L");
        break;
    case 0x35:
        DecString(instr_stream, "(HL)");
        break;
    case 0x3D:
        DecString(instr_stream, "A");
        break;

    // ******** 16-bit arithmetic ********
    // ADD HL, R -- Add the value in the 16-bit register R to HL.
    // Flags:
    //     Z: Unchanged
    //     N: Reset
    //     H: Set if carry from bit 11
    //     C: Set if carry from bit 15
    case 0x09:
        AddString(instr_stream, "HL", "BC");
        break;
    case 0x19:
        AddString(instr_stream, "HL", "DE");
        break;
    case 0x29:
        AddString(instr_stream, "HL", "HL");
        break;
    case 0x39:
        AddString(instr_stream, "HL", "SP");
        break;
    // ADD SP, n -- Add signed immediate byte to SP.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Set appropriately, with immediate as unsigned byte.
    //     C: Set appropriately, with immediate as unsigned byte.
    case 0xE8:
        AddString(instr_stream, "SP", NextSignedByteAsStr(mem, pc));
        break;
    // INC R -- Increment the value in the 16-bit register R.
    // Flags unchanged
    case 0x03:
        IncString(instr_stream, "BC");
        break;
    case 0x13:
        IncString(instr_stream, "DE");
        break;
    case 0x23:
        IncString(instr_stream, "HL");
        break;
    case 0x33:
        IncString(instr_stream, "SP");
        break;
    // DEC R -- Decrement the value in the 16-bit register R.
    // Flags unchanged
    case 0x0B:
        DecString(instr_stream, "BC");
        break;
    case 0x1B:
        DecString(instr_stream, "DE");
        break;
    case 0x2B:
        DecString(instr_stream, "HL");
        break;
    case 0x3B:
        DecString(instr_stream, "SP");
        break;

    // ******** Miscellaneous Arithmetic ********
    // DAA -- Encode the contents of A in BCD.
    // Flags:
    //     Z: Set if result is zero
    //     N: Unchanged
    //     H: Reset
    //     C: Set appropriately
    case 0x27:
        instr_stream << "DAA";
        break;
    // CPL -- Complement the value in register A.
    // Flags:
    //     Z: Unchanged
    //     N: Set
    //     H: Set
    //     C: Unchanged
    case 0x2F:
        instr_stream << "CPL";
        break;
    // SCF -- Set the carry flag.
    // Flags:
    //     Z: Unchanged
    //     N: Reset
    //     H: Reset
    //     C: Set
    case 0x37:
        instr_stream << "SCF";
        break;
    // CCF -- Complement the carry flag.
    // Flags:
    //     Z: Unchanged
    //     N: Reset
    //     H: Reset
    //     C: Complemented
    case 0x3F:
        instr_stream << "CCF";
        break;

    // ******** Rotates and Shifts ********
    // RLCA -- Left rotate A.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 7 before the rotate
    case 0x07:
        instr_stream << "RLCA";
        break;
    // RLA -- Left rotate A through the carry flag.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 7 before the rotate
    case 0x17:
        instr_stream << "RLA";
        break;
    // RRCA -- Right rotate A.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 0 before the rotate
    case 0x0F:
        instr_stream << "RRCA";
        break;
    // RRA -- Right rotate A through the carry flag.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 0 before the rotate
    case 0x1F:
        instr_stream << "RRA";
        break;

    // ******** Jumps ********
    // JP nn -- Jump to the address given by the 16-bit immediate value.
    case 0xC3:
        JumpString(instr_stream, NextWordAsStr(mem, pc));
        break;
    // JP cc, nn -- Jump to the address given by the 16-bit immediate value if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0xC2:
        JumpString(instr_stream, "NZ", NextWordAsStr(mem, pc));
        break;
    case 0xCA:
        JumpString(instr_stream, "Z", NextWordAsStr(mem, pc));
        break;
    case 0xD2:
        JumpString(instr_stream, "NC", NextWordAsStr(mem, pc));
        break;
    case 0xDA:
        JumpString(instr_stream, "C", NextWordAsStr(mem, pc));
        break;
    // JP (HL) -- Jump to the address contained in HL.
    case 0xE9:
        JumpString(instr_stream, "HL");
        break;
    // JR n -- Jump to the current address + immediate signed byte.
    case 0x18:
        RelJumpString(instr_stream, NextSignedByteAsStr(mem, pc));
        break;
    // JR cc, n -- Jump to the current address + immediate signed byte if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0x20:
        RelJumpString(instr_stream, "NZ", NextSignedByteAsStr(mem, pc));
        break;
    case 0x28:
        RelJumpString(instr_stream, "Z", NextSignedByteAsStr(mem, pc));
        break;
    case 0x30:
        RelJumpString(instr_stream, "NC", NextSignedByteAsStr(mem, pc));
        break;
    case 0x38:
        RelJumpString(instr_stream, "C", NextSignedByteAsStr(mem, pc));
        break;

    // ******** Calls ********
    // CALL nn -- Push address of the next instruction onto the stack, and jump to the address given by 
    // the 16-bit immediate value.
    case 0xCD:
        CallString(instr_stream, NextWordAsStr(mem, pc));
        break;
    // CALL cc, nn -- Push address of the next instruction onto the stack, and jump to the address given by 
    // the 16-bit immediate value, if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0xC4:
        CallString(instr_stream, "NZ", NextWordAsStr(mem, pc+1));
        break;
    case 0xCC:
        CallString(instr_stream, "Z", NextWordAsStr(mem, pc+1));
        break;
    case 0xD4:
        CallString(instr_stream, "NC", NextWordAsStr(mem, pc+1));
        break;
    case 0xDC:
        CallString(instr_stream, "C", NextWordAsStr(mem, pc+1));
        break;

    // ******** Returns ********
    // RET -- Pop two bytes off the stack and jump to their effective address.
    case 0xC9:
        ReturnInterruptString(instr_stream, "");
        break;
    // RET cc -- Pop two bytes off the stack and jump to their effective address, if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0xC0:
        ReturnCondString(instr_stream, "NZ");
        break;
    case 0xC8:
        ReturnCondString(instr_stream, "Z");
        break;
    case 0xD0:
        ReturnCondString(instr_stream, "NC");
        break;
    case 0xD8:
        ReturnCondString(instr_stream, "C");
        break;
    // RETI -- Pop two bytes off the stack and jump to their effective address, and enable interrupts.
    case 0xD9:
        ReturnInterruptString(instr_stream, "I");
        break;

    // ******** Restarts ********
    // RST n -- Push address of next instruction onto the stack, and jump to the
    // address given by n.
    case 0xC7:
        RestartString(instr_stream, "0x0000");
        break;
    case 0xCF:
        RestartString(instr_stream, "0x0008");
        break;
    case 0xD7:
        RestartString(instr_stream, "0x0010");
        break;
    case 0xDF:
        RestartString(instr_stream, "0x0018");
        break;
    case 0xE7:
        RestartString(instr_stream, "0x0020");
        break;
    case 0xEF:
        RestartString(instr_stream, "0x0028");
        break;
    case 0xF7:
        RestartString(instr_stream, "0x0030");
        break;
    case 0xFF:
        RestartString(instr_stream, "0x0038");
        break;

    // ******** System Control ********
    // NOP -- No operation.
    case 0x00:
        instr_stream << "NOP";
        break;
    // HALT -- Put CPU into lower power mode until an interrupt occurs.
    case 0x76:
        instr_stream << "HALT";
        break;
    // STOP -- Halt both the CPU and LCD until a button is pressed.
    case 0x10:
        instr_stream << "STOP " << NextByteAsStr(mem, pc);
        break;
    // DI -- Disable interrupts.
    case 0xF3:
        instr_stream << "DI";
        break;
    // EI -- Enable interrupts after the next instruction is executed.
    case 0xFB:
        instr_stream << "EI";
        break;

    // ******** CB prefix opcodes ********
    case 0xCB:
        // Get opcode suffix from next byte.
        switch (mem.ReadMem8(pc+1)) {
        // ******** Rotates and Shifts ********
        // RLC R -- Left rotate the value in register R.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 7 before the rotate
        case 0x00:
            RotLeftString(instr_stream, "C", "B");
            break;
        case 0x01:
            RotLeftString(instr_stream, "C", "C");
            break;
        case 0x02:
            RotLeftString(instr_stream, "C", "D");
            break;
        case 0x03:
            RotLeftString(instr_stream, "C", "E");
            break;
        case 0x04:
            RotLeftString(instr_stream, "C", "H");
            break;
        case 0x05:
            RotLeftString(instr_stream, "C", "L");
            break;
        case 0x06:
            RotLeftString(instr_stream, "C", "(HL)");
            break;
        case 0x07:
            RotLeftString(instr_stream, "C", "A");
            break;
        // RL R -- Left rotate the value in register R through the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 7 before the rotate
        case 0x10:
            RotLeftString(instr_stream, "", "B");
            break;
        case 0x11:
            RotLeftString(instr_stream, "", "C");
            break;
        case 0x12:
            RotLeftString(instr_stream, "", "D");
            break;
        case 0x13:
            RotLeftString(instr_stream, "", "E");
            break;
        case 0x14:
            RotLeftString(instr_stream, "", "H");
            break;
        case 0x15:
            RotLeftString(instr_stream, "", "L");
            break;
        case 0x16:
            RotLeftString(instr_stream, "", "(HL)");
            break;
        case 0x17:
            RotLeftString(instr_stream, "", "A");
            break;
        // RRC R -- Right rotate the value in register R.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x08:
            RotRightString(instr_stream, "C", "B");
            break;
        case 0x09:
            RotRightString(instr_stream, "C", "C");
            break;
        case 0x0A:
            RotRightString(instr_stream, "C", "D");
            break;
        case 0x0B:
            RotRightString(instr_stream, "C", "E");
            break;
        case 0x0C:
            RotRightString(instr_stream, "C", "H");
            break;
        case 0x0D:
            RotRightString(instr_stream, "C", "L");
            break;
        case 0x0E:
            RotRightString(instr_stream, "C", "(HL)");
            break;
        case 0x0F:
            RotRightString(instr_stream, "C", "A");
            break;
        // RR R -- Right rotate the value in register R through the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x18:
            RotRightString(instr_stream, "", "B");
            break;
        case 0x19:
            RotRightString(instr_stream, "", "C");
            break;
        case 0x1A:
            RotRightString(instr_stream, "", "D");
            break;
        case 0x1B:
            RotRightString(instr_stream, "", "E");
            break;
        case 0x1C:
            RotRightString(instr_stream, "", "H");
            break;
        case 0x1D:
            RotRightString(instr_stream, "", "L");
            break;
        case 0x1E:
            RotRightString(instr_stream, "", "(HL)");
            break;
        case 0x1F:
            RotRightString(instr_stream, "", "A");
            break;
        // SLA R -- Left shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x20:
            ShiftLeftString(instr_stream, "B");
            break;
        case 0x21:
            ShiftLeftString(instr_stream, "C");
            break;
        case 0x22:
            ShiftLeftString(instr_stream, "D");
            break;
        case 0x23:
            ShiftLeftString(instr_stream, "E");
            break;
        case 0x24:
            ShiftLeftString(instr_stream, "H");
            break;
        case 0x25:
            ShiftLeftString(instr_stream, "L");
            break;
        case 0x26:
            ShiftLeftString(instr_stream, "(HL)");
            break;
        case 0x27:
            ShiftLeftString(instr_stream, "A");
            break;
        // SRA R -- Arithmetic right shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x28:
            ShiftRightString(instr_stream, "A", "B");
            break;
        case 0x29:
            ShiftRightString(instr_stream, "A", "C");
            break;
        case 0x2A:
            ShiftRightString(instr_stream, "A", "D");
            break;
        case 0x2B:
            ShiftRightString(instr_stream, "A", "E");
            break;
        case 0x2C:
            ShiftRightString(instr_stream, "A", "H");
            break;
        case 0x2D:
            ShiftRightString(instr_stream, "A", "L");
            break;
        case 0x2E:
            ShiftRightString(instr_stream, "A", "(HL)");
            break;
        case 0x2F:
            ShiftRightString(instr_stream, "A", "A");
            break;
        // SWAP R -- Swap upper and lower nybbles of register R (rotate by 4).
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Reset
        case 0x30:
            SwapString(instr_stream, "B");
            break;
        case 0x31:
            SwapString(instr_stream, "C");
            break;
        case 0x32:
            SwapString(instr_stream, "D");
            break;
        case 0x33:
            SwapString(instr_stream, "E");
            break;
        case 0x34:
            SwapString(instr_stream, "H");
            break;
        case 0x35:
            SwapString(instr_stream, "L");
            break;
        case 0x36:
            SwapString(instr_stream, "(HL)");
            break;
        case 0x37:
            SwapString(instr_stream, "A");
            break;
        // SRL R -- Logical right shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x38:
            ShiftRightString(instr_stream, "L", "B");
            break;
        case 0x39:
            ShiftRightString(instr_stream, "L", "C");
            break;
        case 0x3A:
            ShiftRightString(instr_stream, "L", "D");
            break;
        case 0x3B:
            ShiftRightString(instr_stream, "L", "E");
            break;
        case 0x3C:
            ShiftRightString(instr_stream, "L", "H");
            break;
        case 0x3D:
            ShiftRightString(instr_stream, "L", "L");
            break;
        case 0x3E:
            ShiftRightString(instr_stream, "L", "(HL)");
            break;
        case 0x3F:
            ShiftRightString(instr_stream, "L", "A");
            break;

        // ******** Bit Manipulation ********
        // BIT b, R -- test bit b of the value in register R.
        // Flags:
        //     Z: Set if bit b of R is zero
        //     N: Reset
        //     H: Set
        //     C: Unchanged
        case 0x40:
            TestBitString(instr_stream, "0", "B");
            break;
        case 0x41:
            TestBitString(instr_stream, "0", "C");
            break;
        case 0x42:
            TestBitString(instr_stream, "0", "D");
            break;
        case 0x43:
            TestBitString(instr_stream, "0", "E");
            break;
        case 0x44:
            TestBitString(instr_stream, "0", "H");
            break;
        case 0x45:
            TestBitString(instr_stream, "0", "L");
            break;
        case 0x46:
            TestBitString(instr_stream, "0", "(HL)");
            break;
        case 0x47:
            TestBitString(instr_stream, "0", "A");
            break;
        case 0x48:
            TestBitString(instr_stream, "1", "B");
            break;
        case 0x49:
            TestBitString(instr_stream, "1", "C");
            break;
        case 0x4A:
            TestBitString(instr_stream, "1", "D");
            break;
        case 0x4B:
            TestBitString(instr_stream, "1", "E");
            break;
        case 0x4C:
            TestBitString(instr_stream, "1", "H");
            break;
        case 0x4D:
            TestBitString(instr_stream, "1", "L");
            break;
        case 0x4E:
            TestBitString(instr_stream, "1", "(HL)");
            break;
        case 0x4F:
            TestBitString(instr_stream, "1", "A");
            break;
        case 0x50:
            TestBitString(instr_stream, "2", "B");
            break;
        case 0x51:
            TestBitString(instr_stream, "2", "C");
            break;
        case 0x52:
            TestBitString(instr_stream, "2", "D");
            break;
        case 0x53:
            TestBitString(instr_stream, "2", "E");
            break;
        case 0x54:
            TestBitString(instr_stream, "2", "H");
            break;
        case 0x55:
            TestBitString(instr_stream, "2", "L");
            break;
        case 0x56:
            TestBitString(instr_stream, "2", "(HL)");
            break;
        case 0x57:
            TestBitString(instr_stream, "2", "A");
            break;
        case 0x58:
            TestBitString(instr_stream, "3", "B");
            break;
        case 0x59:
            TestBitString(instr_stream, "3", "C");
            break;
        case 0x5A:
            TestBitString(instr_stream, "3", "D");
            break;
        case 0x5B:
            TestBitString(instr_stream, "3", "E");
            break;
        case 0x5C:
            TestBitString(instr_stream, "3", "H");
            break;
        case 0x5D:
            TestBitString(instr_stream, "3", "L");
            break;
        case 0x5E:
            TestBitString(instr_stream, "3", "(HL)");
            break;
        case 0x5F:
            TestBitString(instr_stream, "3", "A");
            break;
        case 0x60:
            TestBitString(instr_stream, "4", "B");
            break;
        case 0x61:
            TestBitString(instr_stream, "4", "C");
            break;
        case 0x62:
            TestBitString(instr_stream, "4", "D");
            break;
        case 0x63:
            TestBitString(instr_stream, "4", "E");
            break;
        case 0x64:
            TestBitString(instr_stream, "4", "H");
            break;
        case 0x65:
            TestBitString(instr_stream, "4", "L");
            break;
        case 0x66:
            TestBitString(instr_stream, "4", "(HL)");
            break;
        case 0x67:
            TestBitString(instr_stream, "4", "A");
            break;
        case 0x68:
            TestBitString(instr_stream, "5", "B");
            break;
        case 0x69:
            TestBitString(instr_stream, "5", "C");
            break;
        case 0x6A:
            TestBitString(instr_stream, "5", "D");
            break;
        case 0x6B:
            TestBitString(instr_stream, "5", "E");
            break;
        case 0x6C:
            TestBitString(instr_stream, "5", "H");
            break;
        case 0x6D:
            TestBitString(instr_stream, "5", "L");
            break;
        case 0x6E:
            TestBitString(instr_stream, "5", "(HL)");
            break;
        case 0x6F:
            TestBitString(instr_stream, "5", "A");
            break;
        case 0x70:
            TestBitString(instr_stream, "6", "B");
            break;
        case 0x71:
            TestBitString(instr_stream, "6", "C");
            break;
        case 0x72:
            TestBitString(instr_stream, "6", "D");
            break;
        case 0x73:
            TestBitString(instr_stream, "6", "E");
            break;
        case 0x74:
            TestBitString(instr_stream, "6", "H");
            break;
        case 0x75:
            TestBitString(instr_stream, "6", "L");
            break;
        case 0x76:
            TestBitString(instr_stream, "6", "(HL)");
            break;
        case 0x77:
            TestBitString(instr_stream, "6", "A");
            break;
        case 0x78:
            TestBitString(instr_stream, "7", "B");
            break;
        case 0x79:
            TestBitString(instr_stream, "7", "C");
            break;
        case 0x7A:
            TestBitString(instr_stream, "7", "D");
            break;
        case 0x7B:
            TestBitString(instr_stream, "7", "E");
            break;
        case 0x7C:
            TestBitString(instr_stream, "7", "H");
            break;
        case 0x7D:
            TestBitString(instr_stream, "7", "L");
            break;
        case 0x7E:
            TestBitString(instr_stream, "7", "(HL)");
            break;
        case 0x7F:
            TestBitString(instr_stream, "7", "A");
            break;
        // RES b, R -- reset bit b of the value in register R.
        // Flags unchanged
        case 0x80:
            ResetBitString(instr_stream, "0", "B");
            break;
        case 0x81:
            ResetBitString(instr_stream, "0", "C");
            break;
        case 0x82:
            ResetBitString(instr_stream, "0", "D");
            break;
        case 0x83:
            ResetBitString(instr_stream, "0", "E");
            break;
        case 0x84:
            ResetBitString(instr_stream, "0", "H");
            break;
        case 0x85:
            ResetBitString(instr_stream, "0", "L");
            break;
        case 0x86:
            ResetBitString(instr_stream, "0", "(HL)");
            break;
        case 0x87:
            ResetBitString(instr_stream, "0", "A");
            break;
        case 0x88:
            ResetBitString(instr_stream, "1", "B");
            break;
        case 0x89:
            ResetBitString(instr_stream, "1", "C");
            break;
        case 0x8A:
            ResetBitString(instr_stream, "1", "D");
            break;
        case 0x8B:
            ResetBitString(instr_stream, "1", "E");
            break;
        case 0x8C:
            ResetBitString(instr_stream, "1", "H");
            break;
        case 0x8D:
            ResetBitString(instr_stream, "1", "L");
            break;
        case 0x8E:
            ResetBitString(instr_stream, "1", "(HL)");
            break;
        case 0x8F:
            ResetBitString(instr_stream, "1", "A");
            break;
        case 0x90:
            ResetBitString(instr_stream, "2", "B");
            break;
        case 0x91:
            ResetBitString(instr_stream, "2", "C");
            break;
        case 0x92:
            ResetBitString(instr_stream, "2", "D");
            break;
        case 0x93:
            ResetBitString(instr_stream, "2", "E");
            break;
        case 0x94:
            ResetBitString(instr_stream, "2", "H");
            break;
        case 0x95:
            ResetBitString(instr_stream, "2", "L");
            break;
        case 0x96:
            ResetBitString(instr_stream, "2", "(HL)");
            break;
        case 0x97:
            ResetBitString(instr_stream, "2", "A");
            break;
        case 0x98:
            ResetBitString(instr_stream, "3", "B");
            break;
        case 0x99:
            ResetBitString(instr_stream, "3", "C");
            break;
        case 0x9A:
            ResetBitString(instr_stream, "3", "D");
            break;
        case 0x9B:
            ResetBitString(instr_stream, "3", "E");
            break;
        case 0x9C:
            ResetBitString(instr_stream, "3", "H");
            break;
        case 0x9D:
            ResetBitString(instr_stream, "3", "L");
            break;
        case 0x9E:
            ResetBitString(instr_stream, "3", "(HL)");
            break;
        case 0x9F:
            ResetBitString(instr_stream, "3", "A");
            break;
        case 0xA0:
            ResetBitString(instr_stream, "4", "B");
            break;
        case 0xA1:
            ResetBitString(instr_stream, "4", "C");
            break;
        case 0xA2:
            ResetBitString(instr_stream, "4", "D");
            break;
        case 0xA3:
            ResetBitString(instr_stream, "4", "E");
            break;
        case 0xA4:
            ResetBitString(instr_stream, "4", "H");
            break;
        case 0xA5:
            ResetBitString(instr_stream, "4", "L");
            break;
        case 0xA6:
            ResetBitString(instr_stream, "4", "(HL)");
            break;
        case 0xA7:
            ResetBitString(instr_stream, "4", "A");
            break;
        case 0xA8:
            ResetBitString(instr_stream, "5", "B");
            break;
        case 0xA9:
            ResetBitString(instr_stream, "5", "C");
            break;
        case 0xAA:
            ResetBitString(instr_stream, "5", "D");
            break;
        case 0xAB:
            ResetBitString(instr_stream, "5", "E");
            break;
        case 0xAC:
            ResetBitString(instr_stream, "5", "H");
            break;
        case 0xAD:
            ResetBitString(instr_stream, "5", "L");
            break;
        case 0xAE:
            ResetBitString(instr_stream, "5", "(HL)");
            break;
        case 0xAF:
            ResetBitString(instr_stream, "5", "A");
            break;
        case 0xB0:
            ResetBitString(instr_stream, "6", "B");
            break;
        case 0xB1:
            ResetBitString(instr_stream, "6", "C");
            break;
        case 0xB2:
            ResetBitString(instr_stream, "6", "D");
            break;
        case 0xB3:
            ResetBitString(instr_stream, "6", "E");
            break;
        case 0xB4:
            ResetBitString(instr_stream, "6", "H");
            break;
        case 0xB5:
            ResetBitString(instr_stream, "6", "L");
            break;
        case 0xB6:
            ResetBitString(instr_stream, "6", "(HL)");
            break;
        case 0xB7:
            ResetBitString(instr_stream, "6", "A");
            break;
        case 0xB8:
            ResetBitString(instr_stream, "7", "B");
            break;
        case 0xB9:
            ResetBitString(instr_stream, "7", "C");
            break;
        case 0xBA:
            ResetBitString(instr_stream, "7", "D");
            break;
        case 0xBB:
            ResetBitString(instr_stream, "7", "E");
            break;
        case 0xBC:
            ResetBitString(instr_stream, "7", "H");
            break;
        case 0xBD:
            ResetBitString(instr_stream, "7", "L");
            break;
        case 0xBE:
            ResetBitString(instr_stream, "7", "(HL)");
            break;
        case 0xBF:
            ResetBitString(instr_stream, "7", "A");
            break;
        // SET b, R -- set bit b of the value in register R.
        // Flags unchanged
        case 0xC0:
            SetBitString(instr_stream, "0", "B");
            break;
        case 0xC1:
            SetBitString(instr_stream, "0", "C");
            break;
        case 0xC2:
            SetBitString(instr_stream, "0", "D");
            break;
        case 0xC3:
            SetBitString(instr_stream, "0", "E");
            break;
        case 0xC4:
            SetBitString(instr_stream, "0", "H");
            break;
        case 0xC5:
            SetBitString(instr_stream, "0", "L");
            break;
        case 0xC6:
            SetBitString(instr_stream, "0", "(HL)");
            break;
        case 0xC7:
            SetBitString(instr_stream, "0", "A");
            break;
        case 0xC8:
            SetBitString(instr_stream, "1", "B");
            break;
        case 0xC9:
            SetBitString(instr_stream, "1", "C");
            break;
        case 0xCA:
            SetBitString(instr_stream, "1", "D");
            break;
        case 0xCB:
            SetBitString(instr_stream, "1", "E");
            break;
        case 0xCC:
            SetBitString(instr_stream, "1", "H");
            break;
        case 0xCD:
            SetBitString(instr_stream, "1", "L");
            break;
        case 0xCE:
            SetBitString(instr_stream, "1", "(HL)");
            break;
        case 0xCF:
            SetBitString(instr_stream, "1", "A");
            break;
        case 0xD0:
            SetBitString(instr_stream, "2", "B");
            break;
        case 0xD1:
            SetBitString(instr_stream, "2", "C");
            break;
        case 0xD2:
            SetBitString(instr_stream, "2", "D");
            break;
        case 0xD3:
            SetBitString(instr_stream, "2", "E");
            break;
        case 0xD4:
            SetBitString(instr_stream, "2", "H");
            break;
        case 0xD5:
            SetBitString(instr_stream, "2", "L");
            break;
        case 0xD6:
            SetBitString(instr_stream, "2", "(HL)");
            break;
        case 0xD7:
            SetBitString(instr_stream, "2", "A");
            break;
        case 0xD8:
            SetBitString(instr_stream, "3", "B");
            break;
        case 0xD9:
            SetBitString(instr_stream, "3", "C");
            break;
        case 0xDA:
            SetBitString(instr_stream, "3", "D");
            break;
        case 0xDB:
            SetBitString(instr_stream, "3", "E");
            break;
        case 0xDC:
            SetBitString(instr_stream, "3", "H");
            break;
        case 0xDD:
            SetBitString(instr_stream, "3", "L");
            break;
        case 0xDE:
            SetBitString(instr_stream, "3", "(HL)");
            break;
        case 0xDF:
            SetBitString(instr_stream, "3", "A");
            break;
        case 0xE0:
            SetBitString(instr_stream, "4", "B");
            break;
        case 0xE1:
            SetBitString(instr_stream, "4", "C");
            break;
        case 0xE2:
            SetBitString(instr_stream, "4", "D");
            break;
        case 0xE3:
            SetBitString(instr_stream, "4", "E");
            break;
        case 0xE4:
            SetBitString(instr_stream, "4", "H");
            break;
        case 0xE5:
            SetBitString(instr_stream, "4", "L");
            break;
        case 0xE6:
            SetBitString(instr_stream, "4", "(HL)");
            break;
        case 0xE7:
            SetBitString(instr_stream, "4", "A");
            break;
        case 0xE8:
            SetBitString(instr_stream, "5", "B");
            break;
        case 0xE9:
            SetBitString(instr_stream, "5", "C");
            break;
        case 0xEA:
            SetBitString(instr_stream, "5", "D");
            break;
        case 0xEB:
            SetBitString(instr_stream, "5", "E");
            break;
        case 0xEC:
            SetBitString(instr_stream, "5", "H");
            break;
        case 0xED:
            SetBitString(instr_stream, "5", "L");
            break;
        case 0xEE:
            SetBitString(instr_stream, "5", "(HL)");
            break;
        case 0xEF:
            SetBitString(instr_stream, "5", "A");
            break;
        case 0xF0:
            SetBitString(instr_stream, "6", "B");
            break;
        case 0xF1:
            SetBitString(instr_stream, "6", "C");
            break;
        case 0xF2:
            SetBitString(instr_stream, "6", "D");
            break;
        case 0xF3:
            SetBitString(instr_stream, "6", "E");
            break;
        case 0xF4:
            SetBitString(instr_stream, "6", "H");
            break;
        case 0xF5:
            SetBitString(instr_stream, "6", "L");
            break;
        case 0xF6:
            SetBitString(instr_stream, "6", "(HL)");
            break;
        case 0xF7:
            SetBitString(instr_stream, "6", "A");
            break;
        case 0xF8:
            SetBitString(instr_stream, "7", "B");
            break;
        case 0xF9:
            SetBitString(instr_stream, "7", "C");
            break;
        case 0xFA:
            SetBitString(instr_stream, "7", "D");
            break;
        case 0xFB:
            SetBitString(instr_stream, "7", "E");
            break;
        case 0xFC:
            SetBitString(instr_stream, "7", "H");
            break;
        case 0xFD:
            SetBitString(instr_stream, "7", "L");
            break;
        case 0xFE:
            SetBitString(instr_stream, "7", "(HL)");
            break;
        case 0xFF:
            SetBitString(instr_stream, "7", "A");
            break;

        default:
            // Unreachable, every possible case is specified above.
            break;
        }
        break;

    default:
        UnknownOpcodeString(instr_stream, mem, pc);
        break;
    }
    return instr_stream.str();
}

} // End namespace Log
