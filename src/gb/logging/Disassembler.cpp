// This file is a part of Chroma.
// Copyright (C) 2016-2018 Matthew Murray
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

#include "gb/logging/Logging.h"
#include "gb/core/GameBoy.h"
#include "gb/memory/Memory.h"

namespace Gb {

std::string Logging::NextByteAsStr(const u16 pc) const {
    return fmt::format("0x{0:0>2X}", gameboy.mem->ReadMem(pc + 1));
}

std::string Logging::NextSignedByteAsStr(const u16 pc) const {
    const s8 sbyte = gameboy.mem->ReadMem(pc + 1);
    if (sbyte < 0) {
        return fmt::format("-0x{0:0>2X}", (~sbyte) + 1);
    } else {
        return fmt::format("+0x{0:0>2X}", sbyte);
    }
}

std::string Logging::NextWordAsStr(const u16 pc) const {
    return fmt::format("0x{0:0>4X}", (gameboy.mem->ReadMem(pc + 2) << 8) | gameboy.mem->ReadMem(pc + 1));
}

void Logging::LoadString(const std::string& into, const std::string& from) {
    fmt::print(log_stream, "LD {}, {}", into, from);
}

void Logging::LoadIncString(const std::string& into, const std::string& from) {
    fmt::print(log_stream, "LDI {}, {}", into, from);
}

void Logging::LoadDecString(const std::string& into, const std::string& from) {
    fmt::print(log_stream, "LDD {}, {}", into, from);
}

void Logging::LoadHighString(const std::string& into, const std::string& from) {
    fmt::print(log_stream, "LDH {}, {}", into, from);
}

void Logging::PushString(const std::string& reg) {
    fmt::print(log_stream, "PUSH {}", reg);
}

void Logging::PopString(const std::string& reg) {
    fmt::print(log_stream, "POP {}", reg);
}

void Logging::AddString(const std::string& from) {
    fmt::print(log_stream, "ADD A, {}", from);
}

void Logging::AddString(const std::string& into, const std::string& from) {
    fmt::print(log_stream, "ADD {}, {}", into, from);
}

void Logging::AdcString(const std::string& from) {
    fmt::print(log_stream, "ADC A, {}", from);
}

void Logging::SubString(const std::string& from) {
    fmt::print(log_stream, "SUB A, {}", from);
}

void Logging::SbcString(const std::string& from) {
    fmt::print(log_stream, "SBC A, {}", from);
}

void Logging::AndString(const std::string& with) {
    fmt::print(log_stream, "AND {}", with);
}

void Logging::OrString(const std::string& with) {
    fmt::print(log_stream, "OR {}", with);
}

void Logging::XorString(const std::string& with) {
    fmt::print(log_stream, "XOR {}", with);
}

void Logging::CompareString(const std::string& with) {
    fmt::print(log_stream, "CP {}", with);
}

void Logging::IncString(const std::string& reg) {
    fmt::print(log_stream, "INC {}", reg);
}

void Logging::DecString(const std::string& reg) {
    fmt::print(log_stream, "DEC {}", reg);
}

void Logging::JumpString(const std::string& addr) {
    fmt::print(log_stream, "JP {}", addr);
}

void Logging::JumpString(const std::string& cond, const std::string& addr) {
    fmt::print(log_stream, "JP {}, {}", cond, addr);
}

void Logging::RelJumpString(const std::string& addr) {
    fmt::print(log_stream, "JR {}", addr);
}

void Logging::RelJumpString(const std::string& cond, const std::string& addr) {
    fmt::print(log_stream, "JR {}, {}", cond, addr);
}

void Logging::CallString(const std::string& addr) {
    fmt::print(log_stream, "CALL {}", addr);
}

void Logging::CallString(const std::string& cond, const std::string& addr) {
    fmt::print(log_stream, "CALL {}, {}", cond, addr);
}

void Logging::ReturnInterruptString(const std::string& reti) {
    fmt::print(log_stream, "RET{}", reti);
}

void Logging::ReturnCondString(const std::string& cond) {
    fmt::print(log_stream, "RET {}", cond);
}

void Logging::RestartString(const std::string& addr) {
    fmt::print(log_stream, "RST {}", addr);
}

void Logging::RotLeftString(const std::string& carry, const std::string& reg) {
    fmt::print(log_stream, "RL{} {}", carry, reg);
}

void Logging::RotRightString(const std::string& carry, const std::string& reg) {
    fmt::print(log_stream, "RR{} {}", carry, reg);
}

void Logging::ShiftLeftString(const std::string& reg) {
    fmt::print(log_stream, "SLA {}", reg);
}

void Logging::ShiftRightString(const std::string& a_or_l, const std::string& reg) {
    fmt::print(log_stream, "SR{} {}", a_or_l, reg);
}

void Logging::SwapString(const std::string& reg) {
    fmt::print(log_stream, "SWAP {}", reg);
}

void Logging::TestBitString(const std::string& bit, const std::string& reg) {
    fmt::print(log_stream, "BIT {}, {}", bit, reg);
}

void Logging::ResetBitString(const std::string& bit, const std::string& reg) {
    fmt::print(log_stream, "RES {}, {}", bit, reg);
}

void Logging::SetBitString(const std::string& bit, const std::string& reg) {
    fmt::print(log_stream, "SET {}, {}", bit, reg);
}

void Logging::UnknownOpcodeString(const u8 opcode) {
    fmt::print(log_stream, "Unknown Opcode: 0x{0:0>2X}", opcode);
}

void Logging::Disassemble(const u16 pc) {
    fmt::print(log_stream, "0x{0:0>4X}: ", pc);
    switch (gameboy.mem->ReadMem(pc)) {
    // ******** 8-bit loads ********
    // LD R, n -- Load immediate value n into register R
    case 0x06:
        LoadString("B", NextByteAsStr(pc));
        break;
    case 0x0E:
        LoadString("C", NextByteAsStr(pc));
        break;
    case 0x16:
        LoadString("D", NextByteAsStr(pc));
        break;
    case 0x1E:
        LoadString("E", NextByteAsStr(pc));
        break;
    case 0x26:
        LoadString("H", NextByteAsStr(pc));
        break;
    case 0x2E:
        LoadString("L", NextByteAsStr(pc));
        break;
    case 0x3E:
        LoadString("A", NextByteAsStr(pc));
        break;
    // LD A, R2 -- Load value from R2 into A
    case 0x78:
        LoadString("A", "B");
        break;
    case 0x79:
        LoadString("A", "C");
        break;
    case 0x7A:
        LoadString("A", "D");
        break;
    case 0x7B:
        LoadString("A", "E");
        break;
    case 0x7C:
        LoadString("A", "H");
        break;
    case 0x7D:
        LoadString("A", "L");
        break;
    case 0x7E:
        LoadString("A", "(HL)");
        break;
    case 0x7F:
        LoadString("A", "A");
        break;
    // LD B, R2 -- Load value from R2 into B
    case 0x40:
        LoadString("B", "B");
        break;
    case 0x41:
        LoadString("B", "C");
        break;
    case 0x42:
        LoadString("B", "D");
        break;
    case 0x43:
        LoadString("B", "E");
        break;
    case 0x44:
        LoadString("B", "H");
        break;
    case 0x45:
        LoadString("B", "L");
        break;
    case 0x46:
        LoadString("B", "(HL)");
        break;
    case 0x47:
        LoadString("B", "A");
        break;
    // LD C, R2 -- Load value from R2 into C
    case 0x48:
        LoadString("C", "B");
        break;
    case 0x49:
        LoadString("C", "C");
        break;
    case 0x4A:
        LoadString("C", "D");
        break;
    case 0x4B:
        LoadString("C", "E");
        break;
    case 0x4C:
        LoadString("C", "H");
        break;
    case 0x4D:
        LoadString("C", "L");
        break;
    case 0x4E:
        LoadString("C", "(HL)");
        break;
    case 0x4F:
        LoadString("C", "A");
        break;
    // LD D, R2 -- Load value from R2 into D
    case 0x50:
        LoadString("D", "B");
        break;
    case 0x51:
        LoadString("D", "C");
        break;
    case 0x52:
        LoadString("D", "D");
        break;
    case 0x53:
        LoadString("D", "E");
        break;
    case 0x54:
        LoadString("D", "H");
        break;
    case 0x55:
        LoadString("D", "L");
        break;
    case 0x56:
        LoadString("D", "(HL)");
        break;
    case 0x57:
        LoadString("D", "A");
        break;
    // LD E, R2 -- Load value from R2 into E
    case 0x58:
        LoadString("E", "B");
        break;
    case 0x59:
        LoadString("E", "C");
        break;
    case 0x5A:
        LoadString("E", "D");
        break;
    case 0x5B:
        LoadString("E", "E");
        break;
    case 0x5C:
        LoadString("E", "H");
        break;
    case 0x5D:
        LoadString("E", "L");
        break;
    case 0x5E:
        LoadString("E", "(HL)");
        break;
    case 0x5F:
        LoadString("E", "A");
        break;
    // LD H, R2 -- Load value from R2 into H
    case 0x60:
        LoadString("H", "B");
        break;
    case 0x61:
        LoadString("H", "C");
        break;
    case 0x62:
        LoadString("H", "D");
        break;
    case 0x63:
        LoadString("H", "E");
        break;
    case 0x64:
        LoadString("H", "H");
        break;
    case 0x65:
        LoadString("H", "L");
        break;
    case 0x66:
        LoadString("H", "(HL)");
        break;
    case 0x67:
        LoadString("H", "A");
        break;
    // LD L, R2 -- Load value from R2 into L
    case 0x68:
        LoadString("L", "B");
        break;
    case 0x69:
        LoadString("L", "C");
        break;
    case 0x6A:
        LoadString("L", "D");
        break;
    case 0x6B:
        LoadString("L", "E");
        break;
    case 0x6C:
        LoadString("L", "H");
        break;
    case 0x6D:
        LoadString("L", "L");
        break;
    case 0x6E:
        LoadString("L", "(HL)");
        break;
    case 0x6F:
        LoadString("L", "A");
        break;
    // LD (HL), R2 -- Load value from R2 into memory at (HL)
    case 0x70:
        LoadString("(HL)", "B");
        break;
    case 0x71:
        LoadString("(HL)", "C");
        break;
    case 0x72:
        LoadString("(HL)", "D");
        break;
    case 0x73:
        LoadString("(HL)", "E");
        break;
    case 0x74:
        LoadString("(HL)", "H");
        break;
    case 0x75:
        LoadString("(HL)", "L");
        break;
    case 0x77:
        LoadString("(HL)", "A");
        break;
    case 0x36:
        LoadString("(HL)", NextByteAsStr(pc));
        break;
    // LD A, (nn) -- Load value from memory at (nn) into A
    case 0x0A:
        LoadString("A", "(BC)");
        break;
    case 0x1A:
        LoadString("A", "(DE)");
        break;
    case 0xFA:
        LoadString("A", "(" + NextWordAsStr(pc) + ")");
        break;
    // LD (nn), A -- Load value from A into memory at (nn)
    case 0x02:
        LoadString("(BC)", "A");
        break;
    case 0x12:
        LoadString("(DE)", "A");
        break;
    case 0xEA:
        LoadString("(" + NextWordAsStr(pc) + ")", "A");
        break;
    // LD (C), A -- Load value from A into memory at (0xFF00 + C)
    case 0xE2:
        LoadString("(0xFF00 + C)", "A");
        break;
    // LD A, (C) -- Load value from memory at (0xFF00 + C) into A
    case 0xF2:
        LoadString("A", "(0xFF00 + C)");
        break;
    // LDI (HL), A -- Load value from A into memory at (HL), then increment HL
    case 0x22:
        LoadIncString("(HL)", "A");
        break;
    // LDI A, (HL) -- Load value from memory at (HL) into A, then increment HL
    case 0x2A:
        LoadIncString("A", "(HL)");
        break;
    // LDD (HL), A -- Load value from A into memory at (HL), then decrement HL
    case 0x32:
        LoadDecString("(HL)", "A");
        break;
    // LDD A, (HL) -- Load value from memory at (HL) into A, then decrement HL
    case 0x3A:
        LoadDecString("A", "(HL)");
        break;
    // LDH (n), A -- Load value from A into memory at (0xFF00+n), with n as immediate byte value
    case 0xE0:
        // Take substring to remove the 0x prefix.
        LoadHighString("(0xFF" + NextByteAsStr(pc).substr(2, 2) + ")", "A");
        break;
    // LDH A, (n) -- Load value from memory at (0xFF00+n) into A, with n as immediate byte value 
    case 0xF0:
        // Take substring to remove the 0x prefix.
        LoadHighString("A", "(0xFF" + NextByteAsStr(pc).substr(2, 2) + ")");
        break;

    // ******** 16-bit loads ********
    // LD R, nn -- Load 16-bit immediate value into 16-bit register R
    case 0x01:
        LoadString("BC", NextWordAsStr(pc));
        break;
    case 0x11:
        LoadString("DE", NextWordAsStr(pc));
        break;
    case 0x21:
        LoadString("HL", NextWordAsStr(pc));
        break;
    case 0x31:
        LoadString("SP", NextWordAsStr(pc));
        break;
    // LD SP, HL -- Load value from HL into SP
    case 0xF9:
        LoadString("SP", "HL");
        break;
    // LD HL, SP+n -- Load value from SP + n into HL, with n as signed immediate byte value
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Set appropriately, with immediate as unsigned byte.
    //     C: Set appropriately, with immediate as unsigned byte.
    case 0xF8:
        LoadString("HL", "SP" + NextSignedByteAsStr(pc));
        break;
    // LD (nn), SP -- Load value from SP into memory at (nn)
    case 0x08:
        LoadString("(" + NextWordAsStr(pc) + ")", "SP");
        break;
    // PUSH R -- Push 16-bit register R onto the stack and decrement the stack pointer by 2
    case 0xC5:
        PushString("BC");
        break;
    case 0xD5:
        PushString("DE");
        break;
    case 0xE5:
        PushString("HL");
        break;
    case 0xF5:
        PushString("AF");
        break;
    // POP R -- Pop 2 bytes off the stack into 16-bit register R and increment the stack pointer by 2
    case 0xC1:
        PopString("BC");
        break;
    case 0xD1:
        PopString("DE");
        break;
    case 0xE1:
        PopString("HL");
        break;
    case 0xF1:
        PopString("AF");
        break;

    // ******** 8-bit arithmetic and logic ********
    // ADD A, R -- Add value in register R to A
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Set if carry from bit 7
    case 0x80:
        AddString("B");
        break;
    case 0x81:
        AddString("C");
        break;
    case 0x82:
        AddString("D");
        break;
    case 0x83:
        AddString("E");
        break;
    case 0x84:
        AddString("H");
        break;
    case 0x85:
        AddString("L");
        break;
    case 0x86:
        AddString("(HL)");
        break;
    case 0x87:
        AddString("A");
        break;
    // ADD A, n -- Add immediate value n to A
    // Flags: same as ADD A, R
    case 0xC6:
        AddString(NextByteAsStr(pc));
        break;
    // ADC A, R -- Add value in register R + the carry flag to A
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Set if carry from bit 7
    case 0x88:
        AdcString("B");
        break;
    case 0x89:
        AdcString("C");
        break;
    case 0x8A:
        AdcString("D");
        break;
    case 0x8B:
        AdcString("E");
        break;
    case 0x8C:
        AdcString("H");
        break;
    case 0x8D:
        AdcString("L");
        break;
    case 0x8E:
        AdcString("(HL)");
        break;
    case 0x8F:
        AdcString("A");
        break;
    // ADC A, n -- Add immediate value n + the carry flag to A
    // Flags: same as ADC A, R
    case 0xCE:
        AdcString(NextByteAsStr(pc));
        break;
    // SUB R -- Subtract the value in register R from  A
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0x90:
        SubString("B");
        break;
    case 0x91:
        SubString("C");
        break;
    case 0x92:
        SubString("D");
        break;
    case 0x93:
        SubString("E");
        break;
    case 0x94:
        SubString("H");
        break;
    case 0x95:
        SubString("L");
        break;
    case 0x96:
        SubString("(HL)");
        break;
    case 0x97:
        SubString("A");
        break;
    // SUB n -- Subtract immediate value n from  A
    // Flags: same as SUB R
    case 0xD6:
        SubString(NextByteAsStr(pc));
        break;
    // SBC A, R -- Subtract the value in register R + carry flag from  A
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0x98:
        SbcString("B");
        break;
    case 0x99:
        SbcString("C");
        break;
    case 0x9A:
        SbcString("D");
        break;
    case 0x9B:
        SbcString("E");
        break;
    case 0x9C:
        SbcString("H");
        break;
    case 0x9D:
        SbcString("L");
        break;
    case 0x9E:
        SbcString("(HL)");
        break;
    case 0x9F:
        SbcString("A");
        break;
    // SBC A, n -- Subtract immediate value n + carry flag from  A
    // Flags: same as SBC A, R
    case 0xDE:
        SbcString(NextByteAsStr(pc));
        break;
    // AND R -- Bitwise AND the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set
    //     C: Reset
    case 0xA0:
        AndString("B");
        break;
    case 0xA1:
        AndString("C");
        break;
    case 0xA2:
        AndString("D");
        break;
    case 0xA3:
        AndString("E");
        break;
    case 0xA4:
        AndString("H");
        break;
    case 0xA5:
        AndString("L");
        break;
    case 0xA6:
        AndString("(HL)");
        break;
    case 0xA7:
        AndString("A");
        break;
    // AND n -- Bitwise AND the immediate value with A. 
    // Flags: same as AND R
    case 0xE6:
        AndString(NextByteAsStr(pc));
        break;
    // OR R -- Bitwise OR the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Reset
    //     C: Reset
    case 0xB0:
        OrString("B");
        break;
    case 0xB1:
        OrString("C");
        break;
    case 0xB2:
        OrString("D");
        break;
    case 0xB3:
        OrString("E");
        break;
    case 0xB4:
        OrString("H");
        break;
    case 0xB5:
        OrString("L");
        break;
    case 0xB6:
        OrString("(HL)");
        break;
    case 0xB7:
        OrString("A");
        break;
    // OR n -- Bitwise OR the immediate value with A. 
    // Flags: same as OR R
    case 0xF6:
        OrString(NextByteAsStr(pc));
        break;
    // XOR R -- Bitwise XOR the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Reset
    //     C: Reset
    case 0xA8:
        XorString("B");
        break;
    case 0xA9:
        XorString("C");
        break;
    case 0xAA:
        XorString("D");
        break;
    case 0xAB:
        XorString("E");
        break;
    case 0xAC:
        XorString("H");
        break;
    case 0xAD:
        XorString("L");
        break;
    case 0xAE:
        XorString("(HL)");
        break;
    case 0xAF:
        XorString("A");
        break;
    // XOR n -- Bitwise XOR the immediate value with A. 
    // Flags: same as XOR R
    case 0xEE:
        XorString(NextByteAsStr(pc));
        break;
    // CP R -- Compare A with the value in register R. This performs a subtraction but does not modify A.
    // Flags:
    //     Z: Set if result is zero, i.e. A is equal to R
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0xB8:
        CompareString("B");
        break;
    case 0xB9:
        CompareString("C");
        break;
    case 0xBA:
        CompareString("D");
        break;
    case 0xBB:
        CompareString("E");
        break;
    case 0xBC:
        CompareString("H");
        break;
    case 0xBD:
        CompareString("L");
        break;
    case 0xBE:
        CompareString("(HL)");
        break;
    case 0xBF:
        CompareString("A");
        break;
    // CP n -- Compare A with the immediate value. This performs a subtraction but does not modify A.
    // Flags: same as CP R
    case 0xFE:
        CompareString(NextByteAsStr(pc));
        break;
    // INC R -- Increment the value in register R.
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Unchanged
    case 0x04:
        IncString("B");
        break;
    case 0x0C:
        IncString("C");
        break;
    case 0x14:
        IncString("D");
        break;
    case 0x1C:
        IncString("E");
        break;
    case 0x24:
        IncString("H");
        break;
    case 0x2C:
        IncString("L");
        break;
    case 0x34:
        IncString("(HL)");
        break;
    case 0x3C:
        IncString("A");
        break;
    // DEC R -- Decrement the value in register R.
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Unchanged
    case 0x05:
        DecString("B");
        break;
    case 0x0D:
        DecString("C");
        break;
    case 0x15:
        DecString("D");
        break;
    case 0x1D:
        DecString("E");
        break;
    case 0x25:
        DecString("H");
        break;
    case 0x2D:
        DecString("L");
        break;
    case 0x35:
        DecString("(HL)");
        break;
    case 0x3D:
        DecString("A");
        break;

    // ******** 16-bit arithmetic ********
    // ADD HL, R -- Add the value in the 16-bit register R to HL.
    // Flags:
    //     Z: Unchanged
    //     N: Reset
    //     H: Set if carry from bit 11
    //     C: Set if carry from bit 15
    case 0x09:
        AddString("HL", "BC");
        break;
    case 0x19:
        AddString("HL", "DE");
        break;
    case 0x29:
        AddString("HL", "HL");
        break;
    case 0x39:
        AddString("HL", "SP");
        break;
    // ADD SP, n -- Add signed immediate byte to SP.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Set appropriately, with immediate as unsigned byte.
    //     C: Set appropriately, with immediate as unsigned byte.
    case 0xE8:
        AddString("SP", NextSignedByteAsStr(pc));
        break;
    // INC R -- Increment the value in the 16-bit register R.
    // Flags unchanged
    case 0x03:
        IncString("BC");
        break;
    case 0x13:
        IncString("DE");
        break;
    case 0x23:
        IncString("HL");
        break;
    case 0x33:
        IncString("SP");
        break;
    // DEC R -- Decrement the value in the 16-bit register R.
    // Flags unchanged
    case 0x0B:
        DecString("BC");
        break;
    case 0x1B:
        DecString("DE");
        break;
    case 0x2B:
        DecString("HL");
        break;
    case 0x3B:
        DecString("SP");
        break;

    // ******** Miscellaneous Arithmetic ********
    // DAA -- Encode the contents of A in BCD.
    // Flags:
    //     Z: Set if result is zero
    //     N: Unchanged
    //     H: Reset
    //     C: Set appropriately
    case 0x27:
        fmt::print(log_stream, "DAA");
        break;
    // CPL -- Complement the value in register A.
    // Flags:
    //     Z: Unchanged
    //     N: Set
    //     H: Set
    //     C: Unchanged
    case 0x2F:
        fmt::print(log_stream, "CPL");
        break;
    // SCF -- Set the carry flag.
    // Flags:
    //     Z: Unchanged
    //     N: Reset
    //     H: Reset
    //     C: Set
    case 0x37:
        fmt::print(log_stream, "SCF");
        break;
    // CCF -- Complement the carry flag.
    // Flags:
    //     Z: Unchanged
    //     N: Reset
    //     H: Reset
    //     C: Complemented
    case 0x3F:
        fmt::print(log_stream, "CCF");
        break;

    // ******** Rotates and Shifts ********
    // RLCA -- Left rotate A.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 7 before the rotate
    case 0x07:
        fmt::print(log_stream, "RLCA");
        break;
    // RLA -- Left rotate A through the carry flag.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 7 before the rotate
    case 0x17:
        fmt::print(log_stream, "RLA");
        break;
    // RRCA -- Right rotate A.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 0 before the rotate
    case 0x0F:
        fmt::print(log_stream, "RRCA");
        break;
    // RRA -- Right rotate A through the carry flag.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 0 before the rotate
    case 0x1F:
        fmt::print(log_stream, "RRA");
        break;

    // ******** Jumps ********
    // JP nn -- Jump to the address given by the 16-bit immediate value.
    case 0xC3:
        JumpString(NextWordAsStr(pc));
        break;
    // JP cc, nn -- Jump to the address given by the 16-bit immediate value if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0xC2:
        JumpString("NZ", NextWordAsStr(pc));
        break;
    case 0xCA:
        JumpString("Z", NextWordAsStr(pc));
        break;
    case 0xD2:
        JumpString("NC", NextWordAsStr(pc));
        break;
    case 0xDA:
        JumpString("C", NextWordAsStr(pc));
        break;
    // JP (HL) -- Jump to the address contained in HL.
    case 0xE9:
        JumpString("HL");
        break;
    // JR n -- Jump to the current address + immediate signed byte.
    case 0x18:
        RelJumpString(NextSignedByteAsStr(pc));
        break;
    // JR cc, n -- Jump to the current address + immediate signed byte if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0x20:
        RelJumpString("NZ", NextSignedByteAsStr(pc));
        break;
    case 0x28:
        RelJumpString("Z", NextSignedByteAsStr(pc));
        break;
    case 0x30:
        RelJumpString("NC", NextSignedByteAsStr(pc));
        break;
    case 0x38:
        RelJumpString("C", NextSignedByteAsStr(pc));
        break;

    // ******** Calls ********
    // CALL nn -- Push address of the next instruction onto the stack, and jump to the address given by 
    // the 16-bit immediate value.
    case 0xCD:
        CallString(NextWordAsStr(pc));
        break;
    // CALL cc, nn -- Push address of the next instruction onto the stack, and jump to the address given by 
    // the 16-bit immediate value, if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0xC4:
        CallString("NZ", NextWordAsStr(pc));
        break;
    case 0xCC:
        CallString("Z", NextWordAsStr(pc));
        break;
    case 0xD4:
        CallString("NC", NextWordAsStr(pc));
        break;
    case 0xDC:
        CallString("C", NextWordAsStr(pc));
        break;

    // ******** Returns ********
    // RET -- Pop two bytes off the stack and jump to their effective address.
    case 0xC9:
        ReturnInterruptString("");
        break;
    // RET cc -- Pop two bytes off the stack and jump to their effective address, if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0xC0:
        ReturnCondString("NZ");
        break;
    case 0xC8:
        ReturnCondString("Z");
        break;
    case 0xD0:
        ReturnCondString("NC");
        break;
    case 0xD8:
        ReturnCondString("C");
        break;
    // RETI -- Pop two bytes off the stack and jump to their effective address, and enable interrupts.
    case 0xD9:
        ReturnInterruptString("I");
        break;

    // ******** Restarts ********
    // RST n -- Push address of next instruction onto the stack, and jump to the
    // address given by n.
    case 0xC7:
        RestartString("0x0000");
        break;
    case 0xCF:
        RestartString("0x0008");
        break;
    case 0xD7:
        RestartString("0x0010");
        break;
    case 0xDF:
        RestartString("0x0018");
        break;
    case 0xE7:
        RestartString("0x0020");
        break;
    case 0xEF:
        RestartString("0x0028");
        break;
    case 0xF7:
        RestartString("0x0030");
        break;
    case 0xFF:
        RestartString("0x0038");
        break;

    // ******** System Control ********
    // NOP -- No operation.
    case 0x00:
        fmt::print(log_stream, "NOP");
        break;
    // HALT -- Put CPU into lower power mode until an interrupt occurs.
    case 0x76:
        fmt::print(log_stream, "HALT");
        break;
    // STOP -- Halt both the CPU and LCD until a button is pressed.
    case 0x10:
        fmt::print(log_stream, "STOP {}", NextByteAsStr(pc));
        break;
    // DI -- Disable interrupts.
    case 0xF3:
        fmt::print(log_stream, "DI");
        break;
    // EI -- Enable interrupts after the next instruction is executed.
    case 0xFB:
        fmt::print(log_stream, "EI");
        break;

    // ******** CB prefix opcodes ********
    case 0xCB:
        // Get opcode suffix from next byte.
        switch (gameboy.mem->ReadMem(pc + 1)) {
        // ******** Rotates and Shifts ********
        // RLC R -- Left rotate the value in register R.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 7 before the rotate
        case 0x00:
            RotLeftString("C", "B");
            break;
        case 0x01:
            RotLeftString("C", "C");
            break;
        case 0x02:
            RotLeftString("C", "D");
            break;
        case 0x03:
            RotLeftString("C", "E");
            break;
        case 0x04:
            RotLeftString("C", "H");
            break;
        case 0x05:
            RotLeftString("C", "L");
            break;
        case 0x06:
            RotLeftString("C", "(HL)");
            break;
        case 0x07:
            RotLeftString("C", "A");
            break;
        // RL R -- Left rotate the value in register R through the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 7 before the rotate
        case 0x10:
            RotLeftString("", "B");
            break;
        case 0x11:
            RotLeftString("", "C");
            break;
        case 0x12:
            RotLeftString("", "D");
            break;
        case 0x13:
            RotLeftString("", "E");
            break;
        case 0x14:
            RotLeftString("", "H");
            break;
        case 0x15:
            RotLeftString("", "L");
            break;
        case 0x16:
            RotLeftString("", "(HL)");
            break;
        case 0x17:
            RotLeftString("", "A");
            break;
        // RRC R -- Right rotate the value in register R.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x08:
            RotRightString("C", "B");
            break;
        case 0x09:
            RotRightString("C", "C");
            break;
        case 0x0A:
            RotRightString("C", "D");
            break;
        case 0x0B:
            RotRightString("C", "E");
            break;
        case 0x0C:
            RotRightString("C", "H");
            break;
        case 0x0D:
            RotRightString("C", "L");
            break;
        case 0x0E:
            RotRightString("C", "(HL)");
            break;
        case 0x0F:
            RotRightString("C", "A");
            break;
        // RR R -- Right rotate the value in register R through the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x18:
            RotRightString("", "B");
            break;
        case 0x19:
            RotRightString("", "C");
            break;
        case 0x1A:
            RotRightString("", "D");
            break;
        case 0x1B:
            RotRightString("", "E");
            break;
        case 0x1C:
            RotRightString("", "H");
            break;
        case 0x1D:
            RotRightString("", "L");
            break;
        case 0x1E:
            RotRightString("", "(HL)");
            break;
        case 0x1F:
            RotRightString("", "A");
            break;
        // SLA R -- Left shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x20:
            ShiftLeftString("B");
            break;
        case 0x21:
            ShiftLeftString("C");
            break;
        case 0x22:
            ShiftLeftString("D");
            break;
        case 0x23:
            ShiftLeftString("E");
            break;
        case 0x24:
            ShiftLeftString("H");
            break;
        case 0x25:
            ShiftLeftString("L");
            break;
        case 0x26:
            ShiftLeftString("(HL)");
            break;
        case 0x27:
            ShiftLeftString("A");
            break;
        // SRA R -- Arithmetic right shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x28:
            ShiftRightString("A", "B");
            break;
        case 0x29:
            ShiftRightString("A", "C");
            break;
        case 0x2A:
            ShiftRightString("A", "D");
            break;
        case 0x2B:
            ShiftRightString("A", "E");
            break;
        case 0x2C:
            ShiftRightString("A", "H");
            break;
        case 0x2D:
            ShiftRightString("A", "L");
            break;
        case 0x2E:
            ShiftRightString("A", "(HL)");
            break;
        case 0x2F:
            ShiftRightString("A", "A");
            break;
        // SWAP R -- Swap upper and lower nybbles of register R (rotate by 4).
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Reset
        case 0x30:
            SwapString("B");
            break;
        case 0x31:
            SwapString("C");
            break;
        case 0x32:
            SwapString("D");
            break;
        case 0x33:
            SwapString("E");
            break;
        case 0x34:
            SwapString("H");
            break;
        case 0x35:
            SwapString("L");
            break;
        case 0x36:
            SwapString("(HL)");
            break;
        case 0x37:
            SwapString("A");
            break;
        // SRL R -- Logical right shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x38:
            ShiftRightString("L", "B");
            break;
        case 0x39:
            ShiftRightString("L", "C");
            break;
        case 0x3A:
            ShiftRightString("L", "D");
            break;
        case 0x3B:
            ShiftRightString("L", "E");
            break;
        case 0x3C:
            ShiftRightString("L", "H");
            break;
        case 0x3D:
            ShiftRightString("L", "L");
            break;
        case 0x3E:
            ShiftRightString("L", "(HL)");
            break;
        case 0x3F:
            ShiftRightString("L", "A");
            break;

        // ******** Bit Manipulation ********
        // BIT b, R -- test bit b of the value in register R.
        // Flags:
        //     Z: Set if bit b of R is zero
        //     N: Reset
        //     H: Set
        //     C: Unchanged
        case 0x40:
            TestBitString("0", "B");
            break;
        case 0x41:
            TestBitString("0", "C");
            break;
        case 0x42:
            TestBitString("0", "D");
            break;
        case 0x43:
            TestBitString("0", "E");
            break;
        case 0x44:
            TestBitString("0", "H");
            break;
        case 0x45:
            TestBitString("0", "L");
            break;
        case 0x46:
            TestBitString("0", "(HL)");
            break;
        case 0x47:
            TestBitString("0", "A");
            break;
        case 0x48:
            TestBitString("1", "B");
            break;
        case 0x49:
            TestBitString("1", "C");
            break;
        case 0x4A:
            TestBitString("1", "D");
            break;
        case 0x4B:
            TestBitString("1", "E");
            break;
        case 0x4C:
            TestBitString("1", "H");
            break;
        case 0x4D:
            TestBitString("1", "L");
            break;
        case 0x4E:
            TestBitString("1", "(HL)");
            break;
        case 0x4F:
            TestBitString("1", "A");
            break;
        case 0x50:
            TestBitString("2", "B");
            break;
        case 0x51:
            TestBitString("2", "C");
            break;
        case 0x52:
            TestBitString("2", "D");
            break;
        case 0x53:
            TestBitString("2", "E");
            break;
        case 0x54:
            TestBitString("2", "H");
            break;
        case 0x55:
            TestBitString("2", "L");
            break;
        case 0x56:
            TestBitString("2", "(HL)");
            break;
        case 0x57:
            TestBitString("2", "A");
            break;
        case 0x58:
            TestBitString("3", "B");
            break;
        case 0x59:
            TestBitString("3", "C");
            break;
        case 0x5A:
            TestBitString("3", "D");
            break;
        case 0x5B:
            TestBitString("3", "E");
            break;
        case 0x5C:
            TestBitString("3", "H");
            break;
        case 0x5D:
            TestBitString("3", "L");
            break;
        case 0x5E:
            TestBitString("3", "(HL)");
            break;
        case 0x5F:
            TestBitString("3", "A");
            break;
        case 0x60:
            TestBitString("4", "B");
            break;
        case 0x61:
            TestBitString("4", "C");
            break;
        case 0x62:
            TestBitString("4", "D");
            break;
        case 0x63:
            TestBitString("4", "E");
            break;
        case 0x64:
            TestBitString("4", "H");
            break;
        case 0x65:
            TestBitString("4", "L");
            break;
        case 0x66:
            TestBitString("4", "(HL)");
            break;
        case 0x67:
            TestBitString("4", "A");
            break;
        case 0x68:
            TestBitString("5", "B");
            break;
        case 0x69:
            TestBitString("5", "C");
            break;
        case 0x6A:
            TestBitString("5", "D");
            break;
        case 0x6B:
            TestBitString("5", "E");
            break;
        case 0x6C:
            TestBitString("5", "H");
            break;
        case 0x6D:
            TestBitString("5", "L");
            break;
        case 0x6E:
            TestBitString("5", "(HL)");
            break;
        case 0x6F:
            TestBitString("5", "A");
            break;
        case 0x70:
            TestBitString("6", "B");
            break;
        case 0x71:
            TestBitString("6", "C");
            break;
        case 0x72:
            TestBitString("6", "D");
            break;
        case 0x73:
            TestBitString("6", "E");
            break;
        case 0x74:
            TestBitString("6", "H");
            break;
        case 0x75:
            TestBitString("6", "L");
            break;
        case 0x76:
            TestBitString("6", "(HL)");
            break;
        case 0x77:
            TestBitString("6", "A");
            break;
        case 0x78:
            TestBitString("7", "B");
            break;
        case 0x79:
            TestBitString("7", "C");
            break;
        case 0x7A:
            TestBitString("7", "D");
            break;
        case 0x7B:
            TestBitString("7", "E");
            break;
        case 0x7C:
            TestBitString("7", "H");
            break;
        case 0x7D:
            TestBitString("7", "L");
            break;
        case 0x7E:
            TestBitString("7", "(HL)");
            break;
        case 0x7F:
            TestBitString("7", "A");
            break;
        // RES b, R -- reset bit b of the value in register R.
        // Flags unchanged
        case 0x80:
            ResetBitString("0", "B");
            break;
        case 0x81:
            ResetBitString("0", "C");
            break;
        case 0x82:
            ResetBitString("0", "D");
            break;
        case 0x83:
            ResetBitString("0", "E");
            break;
        case 0x84:
            ResetBitString("0", "H");
            break;
        case 0x85:
            ResetBitString("0", "L");
            break;
        case 0x86:
            ResetBitString("0", "(HL)");
            break;
        case 0x87:
            ResetBitString("0", "A");
            break;
        case 0x88:
            ResetBitString("1", "B");
            break;
        case 0x89:
            ResetBitString("1", "C");
            break;
        case 0x8A:
            ResetBitString("1", "D");
            break;
        case 0x8B:
            ResetBitString("1", "E");
            break;
        case 0x8C:
            ResetBitString("1", "H");
            break;
        case 0x8D:
            ResetBitString("1", "L");
            break;
        case 0x8E:
            ResetBitString("1", "(HL)");
            break;
        case 0x8F:
            ResetBitString("1", "A");
            break;
        case 0x90:
            ResetBitString("2", "B");
            break;
        case 0x91:
            ResetBitString("2", "C");
            break;
        case 0x92:
            ResetBitString("2", "D");
            break;
        case 0x93:
            ResetBitString("2", "E");
            break;
        case 0x94:
            ResetBitString("2", "H");
            break;
        case 0x95:
            ResetBitString("2", "L");
            break;
        case 0x96:
            ResetBitString("2", "(HL)");
            break;
        case 0x97:
            ResetBitString("2", "A");
            break;
        case 0x98:
            ResetBitString("3", "B");
            break;
        case 0x99:
            ResetBitString("3", "C");
            break;
        case 0x9A:
            ResetBitString("3", "D");
            break;
        case 0x9B:
            ResetBitString("3", "E");
            break;
        case 0x9C:
            ResetBitString("3", "H");
            break;
        case 0x9D:
            ResetBitString("3", "L");
            break;
        case 0x9E:
            ResetBitString("3", "(HL)");
            break;
        case 0x9F:
            ResetBitString("3", "A");
            break;
        case 0xA0:
            ResetBitString("4", "B");
            break;
        case 0xA1:
            ResetBitString("4", "C");
            break;
        case 0xA2:
            ResetBitString("4", "D");
            break;
        case 0xA3:
            ResetBitString("4", "E");
            break;
        case 0xA4:
            ResetBitString("4", "H");
            break;
        case 0xA5:
            ResetBitString("4", "L");
            break;
        case 0xA6:
            ResetBitString("4", "(HL)");
            break;
        case 0xA7:
            ResetBitString("4", "A");
            break;
        case 0xA8:
            ResetBitString("5", "B");
            break;
        case 0xA9:
            ResetBitString("5", "C");
            break;
        case 0xAA:
            ResetBitString("5", "D");
            break;
        case 0xAB:
            ResetBitString("5", "E");
            break;
        case 0xAC:
            ResetBitString("5", "H");
            break;
        case 0xAD:
            ResetBitString("5", "L");
            break;
        case 0xAE:
            ResetBitString("5", "(HL)");
            break;
        case 0xAF:
            ResetBitString("5", "A");
            break;
        case 0xB0:
            ResetBitString("6", "B");
            break;
        case 0xB1:
            ResetBitString("6", "C");
            break;
        case 0xB2:
            ResetBitString("6", "D");
            break;
        case 0xB3:
            ResetBitString("6", "E");
            break;
        case 0xB4:
            ResetBitString("6", "H");
            break;
        case 0xB5:
            ResetBitString("6", "L");
            break;
        case 0xB6:
            ResetBitString("6", "(HL)");
            break;
        case 0xB7:
            ResetBitString("6", "A");
            break;
        case 0xB8:
            ResetBitString("7", "B");
            break;
        case 0xB9:
            ResetBitString("7", "C");
            break;
        case 0xBA:
            ResetBitString("7", "D");
            break;
        case 0xBB:
            ResetBitString("7", "E");
            break;
        case 0xBC:
            ResetBitString("7", "H");
            break;
        case 0xBD:
            ResetBitString("7", "L");
            break;
        case 0xBE:
            ResetBitString("7", "(HL)");
            break;
        case 0xBF:
            ResetBitString("7", "A");
            break;
        // SET b, R -- set bit b of the value in register R.
        // Flags unchanged
        case 0xC0:
            SetBitString("0", "B");
            break;
        case 0xC1:
            SetBitString("0", "C");
            break;
        case 0xC2:
            SetBitString("0", "D");
            break;
        case 0xC3:
            SetBitString("0", "E");
            break;
        case 0xC4:
            SetBitString("0", "H");
            break;
        case 0xC5:
            SetBitString("0", "L");
            break;
        case 0xC6:
            SetBitString("0", "(HL)");
            break;
        case 0xC7:
            SetBitString("0", "A");
            break;
        case 0xC8:
            SetBitString("1", "B");
            break;
        case 0xC9:
            SetBitString("1", "C");
            break;
        case 0xCA:
            SetBitString("1", "D");
            break;
        case 0xCB:
            SetBitString("1", "E");
            break;
        case 0xCC:
            SetBitString("1", "H");
            break;
        case 0xCD:
            SetBitString("1", "L");
            break;
        case 0xCE:
            SetBitString("1", "(HL)");
            break;
        case 0xCF:
            SetBitString("1", "A");
            break;
        case 0xD0:
            SetBitString("2", "B");
            break;
        case 0xD1:
            SetBitString("2", "C");
            break;
        case 0xD2:
            SetBitString("2", "D");
            break;
        case 0xD3:
            SetBitString("2", "E");
            break;
        case 0xD4:
            SetBitString("2", "H");
            break;
        case 0xD5:
            SetBitString("2", "L");
            break;
        case 0xD6:
            SetBitString("2", "(HL)");
            break;
        case 0xD7:
            SetBitString("2", "A");
            break;
        case 0xD8:
            SetBitString("3", "B");
            break;
        case 0xD9:
            SetBitString("3", "C");
            break;
        case 0xDA:
            SetBitString("3", "D");
            break;
        case 0xDB:
            SetBitString("3", "E");
            break;
        case 0xDC:
            SetBitString("3", "H");
            break;
        case 0xDD:
            SetBitString("3", "L");
            break;
        case 0xDE:
            SetBitString("3", "(HL)");
            break;
        case 0xDF:
            SetBitString("3", "A");
            break;
        case 0xE0:
            SetBitString("4", "B");
            break;
        case 0xE1:
            SetBitString("4", "C");
            break;
        case 0xE2:
            SetBitString("4", "D");
            break;
        case 0xE3:
            SetBitString("4", "E");
            break;
        case 0xE4:
            SetBitString("4", "H");
            break;
        case 0xE5:
            SetBitString("4", "L");
            break;
        case 0xE6:
            SetBitString("4", "(HL)");
            break;
        case 0xE7:
            SetBitString("4", "A");
            break;
        case 0xE8:
            SetBitString("5", "B");
            break;
        case 0xE9:
            SetBitString("5", "C");
            break;
        case 0xEA:
            SetBitString("5", "D");
            break;
        case 0xEB:
            SetBitString("5", "E");
            break;
        case 0xEC:
            SetBitString("5", "H");
            break;
        case 0xED:
            SetBitString("5", "L");
            break;
        case 0xEE:
            SetBitString("5", "(HL)");
            break;
        case 0xEF:
            SetBitString("5", "A");
            break;
        case 0xF0:
            SetBitString("6", "B");
            break;
        case 0xF1:
            SetBitString("6", "C");
            break;
        case 0xF2:
            SetBitString("6", "D");
            break;
        case 0xF3:
            SetBitString("6", "E");
            break;
        case 0xF4:
            SetBitString("6", "H");
            break;
        case 0xF5:
            SetBitString("6", "L");
            break;
        case 0xF6:
            SetBitString("6", "(HL)");
            break;
        case 0xF7:
            SetBitString("6", "A");
            break;
        case 0xF8:
            SetBitString("7", "B");
            break;
        case 0xF9:
            SetBitString("7", "C");
            break;
        case 0xFA:
            SetBitString("7", "D");
            break;
        case 0xFB:
            SetBitString("7", "E");
            break;
        case 0xFC:
            SetBitString("7", "H");
            break;
        case 0xFD:
            SetBitString("7", "L");
            break;
        case 0xFE:
            SetBitString("7", "(HL)");
            break;
        case 0xFF:
            SetBitString("7", "A");
            break;

        default:
            // Unreachable, every possible case is specified above.
            break;
        }
        break;

    default:
        UnknownOpcodeString(gameboy.mem->ReadMem(pc));
        break;
    }

    fmt::print(log_stream, "\n");
}

} // End namespace Gb
