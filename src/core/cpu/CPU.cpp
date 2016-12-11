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

#include "core/cpu/CPU.h"

namespace Core {

CPU::CPU(Memory& memory, Timer& tima, LCD& display, Serial& serial_io)
        : mem(memory)
        , timer(tima)
        , lcd(display)
        , serial(serial_io) {

    // Initial register values
    if (mem.game_mode == GameMode::DMG) {
        if (mem.console == Console::DMG) {
            a = 0x01; f.bits = 0xB0;
            b = 0x00; c = 0x13;
            d = 0x00; e = 0xD8;
            h = 0x01; l = 0x4D;
        } else if (mem.console == Console::CGB) {
            a = 0x11; f.bits = 0x80;
            b = 0x00; c = 0x00;
            d = 0x00; e = 0x08;
            h = 0x00; l = 0x7C;
        }
    } else if (mem.game_mode == GameMode::CGB) {
        a = 0x11; f.bits = 0x80;
        b = 0x00; c = 0x00;
        d = 0xFF; e = 0x56;
        h = 0x00; l = 0x0D;
    }
}

// Returns the value stored in an 8-bit register.
u8 CPU::Read8(Reg8 R) const {
    switch (R) {
    case (Reg8::A):
        return a;
    case (Reg8::B):
        return b;
    case (Reg8::C):
        return c;
    case (Reg8::D):
        return d;
    case (Reg8::E):
        return e;
    case (Reg8::H):
        return h;
    case (Reg8::L):
        return l;
    default:
        // Unreachable
        return 0x00;
    }
}

// Returns the value stored in a 16-bit register pair.
u16 CPU::Read16(Reg16 R) const {
    switch (R) {
    case (Reg16::AF):
        return (static_cast<u16>(a) << 8) | static_cast<u16>(f.bits);
    case (Reg16::BC):
        return (static_cast<u16>(b) << 8) | static_cast<u16>(c);
    case (Reg16::DE):
        return (static_cast<u16>(d) << 8) | static_cast<u16>(e);
    case (Reg16::HL):
        return (static_cast<u16>(h) << 8) | static_cast<u16>(l);
    case (Reg16::SP):
        return sp;
    default:
        // Unreachable
        return 0x0000;
    }
}

// Writes an 8-bit value to an 8-bit register.
void CPU::Write8(Reg8 R, u8 val) {
    switch (R) {
    case (Reg8::A):
        a = val;
        break;
    case (Reg8::B):
        b = val;
        break;
    case (Reg8::C):
        c = val;
        break;
    case (Reg8::D):
        d = val;
        break;
    case (Reg8::E):
        e = val;
        break;
    case (Reg8::H):
        h = val;
        break;
    case (Reg8::L):
        l = val;
        break;
    default:
        // Unreachable
        break;
    }
}

// Writes a 16-bit value to a 16-bit register pair.
void CPU::Write16(Reg16 R, u16 val) {
    switch (R) {
    case (Reg16::AF):
        a = static_cast<u8>(val >> 8);
        // The lower nybble of f is always 0, even if a pop attempts to write a byte with a nonzero lower nybble.
        f.bits = static_cast<u8>(val) & 0xF0;
        break;
    case (Reg16::BC):
        b = static_cast<u8>(val >> 8);
        c = static_cast<u8>(val);
        break;
    case (Reg16::DE):
        d = static_cast<u8>(val >> 8);
        e = static_cast<u8>(val);
        break;
    case (Reg16::HL):
        h = static_cast<u8>(val >> 8);
        l = static_cast<u8>(val);
        break;
    case (Reg16::SP):
        sp = val;
        break;
    default:
        // Unreachable
        break;
    }
}

// Returns the value from memory at the address stored in the 16-bit register pair HL.
u8 CPU::ReadMemAtHL() {
    const u8 data = mem.ReadMem8((static_cast<u16>(h) << 8) | static_cast<u16>(l));
    HardwareTick(4);
    return data;
}

// Writes a value to memory at the address stored in the 16-bit register pair HL.
void CPU::WriteMemAtHL(const u8 val) {
    mem.WriteMem8((static_cast<u16>(h) << 8) | static_cast<u16>(l), val);
    HardwareTick(4);
}

// Return the byte from memory at the pc and increment the pc.
u8 CPU::GetImmediateByte() {
    const u8 imm = mem.ReadMem8(pc++);
    HardwareTick(4);
    return imm;
}

// Return the signed byte from memory at the pc and increment the pc.
s8 CPU::GetImmediateSignedByte() {
    const s8 imm = static_cast<s8>(mem.ReadMem8(pc++));
    HardwareTick(4);
    return imm;
}

// Return the 16-bit word from memory at the pc and increment the pc by 2.
u16 CPU::GetImmediateWord() {
    const u8 byte_lo = mem.ReadMem8(pc++);
    HardwareTick(4);
    const u8 byte_hi = mem.ReadMem8(pc++);
    HardwareTick(4);

    return (static_cast<u16>(byte_hi) << 8) | static_cast<u16>(byte_lo);
}

// Execute instructions until the specified number of cycles has passed.
void CPU::RunFor(int cycles) {
    while (cycles > 0) {
        cycles -= HandleInterrupts();

//        PrintRegisterState();

        if (cpu_mode == CPUMode::Running) {
            cycles -= ExecuteNext(mem.ReadMem8(pc++));
        } else if (cpu_mode == CPUMode::HaltBug) {
            cycles -= ExecuteNext(mem.ReadMem8(pc));
            cpu_mode = CPUMode::Running;
        } else if (cpu_mode == CPUMode::Halted) {
            HardwareTick(4);
            cycles -= 4;
        }
    }
}

int CPU::HandleInterrupts() {
    if (interrupt_master_enable) {
        if (mem.RequestedEnabledInterrupts()) {
//            PrintInterrupt();
            HardwareTick(12);

            // Disable interrupts, clear the corresponding bit in IF, and jump to the interrupt routine.
            interrupt_master_enable = false;

            if (mem.IsPending(Interrupt::VBLANK)) {
                mem.ClearInterrupt(Interrupt::VBLANK);
                ServiceInterrupt(0x40);
            } else if (mem.IsPending(Interrupt::STAT)) {
                mem.ClearInterrupt(Interrupt::STAT);
                ServiceInterrupt(0x48);
            } else if (mem.IsPending(Interrupt::Timer)) {
                mem.ClearInterrupt(Interrupt::Timer);
                ServiceInterrupt(0x50);
            } else if (mem.IsPending(Interrupt::Serial)) {
                mem.ClearInterrupt(Interrupt::Serial);
                ServiceInterrupt(0x58);
            } else if (mem.IsPending(Interrupt::Joypad)) {
                mem.ClearInterrupt(Interrupt::Joypad);
                ServiceInterrupt(0x60);
            }

            if (cpu_mode == CPUMode::Halted) {
                // Exit halt mode.
                cpu_mode = CPUMode::Running;
                return 24;
            }
            return 20;
        }
    } else if (cpu_mode == CPUMode::Halted) {
        if (mem.RequestedEnabledInterrupts()) {
            // If halt mode is entered when IME is zero, then the next time an interrupt is triggered the CPU does 
            // not jump to the interrupt routine or clear the IF flag. It just exits halt mode and continues execution.
            cpu_mode = CPUMode::Running;
            return 4;
        }
    }

    return 0;
}

void CPU::ServiceInterrupt(const u16 addr) {
    mem.WriteMem8(--sp, static_cast<u8>(pc >> 8));
    HardwareTick(4);

    mem.WriteMem8(--sp, static_cast<u8>(pc));
    HardwareTick(4);

    pc = addr;
}

void CPU::HardwareTick(unsigned int cycles) {
    for (; cycles != 0; cycles -= 4) {

        // Enable interrupts if EI was previously called. 
        interrupt_master_enable = interrupt_master_enable || enable_interrupts_delayed;
        enable_interrupts_delayed = false;

        // Update the rest of the system hardware.
        mem.UpdateOAM_DMA();
        timer.UpdateTimer();
        lcd.UpdateLCD();
        serial.UpdateSerial();

        mem.IF_written_this_cycle = false;

//        timer.PrintRegisterState();
//        lcd.PrintRegisterState();
    }
}

unsigned int CPU::ExecuteNext(const u8 opcode) {
    HardwareTick(4);

    switch (opcode) {
    // ******** 8-bit loads ********
    // LD R, n -- Load immediate value n into register R
    case 0x06:
        Load8(Reg8::B, GetImmediateByte());
        return 8;
    case 0x0E:
        Load8(Reg8::C, GetImmediateByte());
        return 8;
    case 0x16:
        Load8(Reg8::D, GetImmediateByte());
        return 8;
    case 0x1E:
        Load8(Reg8::E, GetImmediateByte());
        return 8;
    case 0x26:
        Load8(Reg8::H, GetImmediateByte());
        return 8;
    case 0x2E:
        Load8(Reg8::L, GetImmediateByte());
        return 8;
    case 0x3E:
        Load8(Reg8::A, GetImmediateByte());
        return 8;
    // LD A, R2 -- Load value from R2 into A
    case 0x78:
        Load8(Reg8::A, b);
        return 4;
    case 0x79:
        Load8(Reg8::A, c);
        return 4;
    case 0x7A:
        Load8(Reg8::A, d);
        return 4;
    case 0x7B:
        Load8(Reg8::A, e);
        return 4;
    case 0x7C:
        Load8(Reg8::A, h);
        return 4;
    case 0x7D:
        Load8(Reg8::A, l);
        return 4;
    case 0x7E:
        Load8FromMemAtHL(Reg8::A);
        return 8;
    case 0x7F:
        Load8(Reg8::A, a);
        return 4;
    // LD B, R2 -- Load value from R2 into B
    case 0x40:
        Load8(Reg8::B, b);
        return 4;
    case 0x41:
        Load8(Reg8::B, c);
        return 4;
    case 0x42:
        Load8(Reg8::B, d);
        return 4;
    case 0x43:
        Load8(Reg8::B, e);
        return 4;
    case 0x44:
        Load8(Reg8::B, h);
        return 4;
    case 0x45:
        Load8(Reg8::B, l);
        return 4;
    case 0x46:
        Load8FromMemAtHL(Reg8::B);
        return 8;
    case 0x47:
        Load8(Reg8::B, a);
        return 4;
    // LD C, R2 -- Load value from R2 into C
    case 0x48:
        Load8(Reg8::C, b);
        return 4;
    case 0x49:
        Load8(Reg8::C, c);
        return 4;
    case 0x4A:
        Load8(Reg8::C, d);
        return 4;
    case 0x4B:
        Load8(Reg8::C, e);
        return 4;
    case 0x4C:
        Load8(Reg8::C, h);
        return 4;
    case 0x4D:
        Load8(Reg8::C, l);
        return 4;
    case 0x4E:
        Load8FromMemAtHL(Reg8::C);
        return 8;
    case 0x4F:
        Load8(Reg8::C, a);
        return 4;
    // LD D, R2 -- Load value from R2 into D
    case 0x50:
        Load8(Reg8::D, b);
        return 4;
    case 0x51:
        Load8(Reg8::D, c);
        return 4;
    case 0x52:
        Load8(Reg8::D, d);
        return 4;
    case 0x53:
        Load8(Reg8::D, e);
        return 4;
    case 0x54:
        Load8(Reg8::D, h);
        return 4;
    case 0x55:
        Load8(Reg8::D, l);
        return 4;
    case 0x56:
        Load8FromMemAtHL(Reg8::D);
        return 8;
    case 0x57:
        Load8(Reg8::D, a);
        return 4;
    // LD E, R2 -- Load value from R2 into E
    case 0x58:
        Load8(Reg8::E, b);
        return 4;
    case 0x59:
        Load8(Reg8::E, c);
        return 4;
    case 0x5A:
        Load8(Reg8::E, d);
        return 4;
    case 0x5B:
        Load8(Reg8::E, e);
        return 4;
    case 0x5C:
        Load8(Reg8::E, h);
        return 4;
    case 0x5D:
        Load8(Reg8::E, l);
        return 4;
    case 0x5E:
        Load8FromMemAtHL(Reg8::E);
        return 8;
    case 0x5F:
        Load8(Reg8::E, a);
        return 4;
    // LD H, R2 -- Load value from R2 into H
    case 0x60:
        Load8(Reg8::H, b);
        return 4;
    case 0x61:
        Load8(Reg8::H, c);
        return 4;
    case 0x62:
        Load8(Reg8::H, d);
        return 4;
    case 0x63:
        Load8(Reg8::H, e);
        return 4;
    case 0x64:
        Load8(Reg8::H, h);
        return 4;
    case 0x65:
        Load8(Reg8::H, l);
        return 4;
    case 0x66:
        Load8FromMemAtHL(Reg8::H);
        return 8;
    case 0x67:
        Load8(Reg8::H, a);
        return 4;
    // LD L, R2 -- Load value from R2 into L
    case 0x68:
        Load8(Reg8::L, b);
        return 4;
    case 0x69:
        Load8(Reg8::L, c);
        return 4;
    case 0x6A:
        Load8(Reg8::L, d);
        return 4;
    case 0x6B:
        Load8(Reg8::L, e);
        return 4;
    case 0x6C:
        Load8(Reg8::L, h);
        return 4;
    case 0x6D:
        Load8(Reg8::L, l);
        return 4;
    case 0x6E:
        Load8FromMemAtHL(Reg8::L);
        return 8;
    case 0x6F:
        Load8(Reg8::L, a);
        return 4;
    // LD (HL), R2 -- Load value from R2 into memory at (HL)
    case 0x70:
        Load8IntoMem(Reg16::HL, b);
        return 8;
    case 0x71:
        Load8IntoMem(Reg16::HL, c);
        return 8;
    case 0x72:
        Load8IntoMem(Reg16::HL, d);
        return 8;
    case 0x73:
        Load8IntoMem(Reg16::HL, e);
        return 8;
    case 0x74:
        Load8IntoMem(Reg16::HL, h);
        return 8;
    case 0x75:
        Load8IntoMem(Reg16::HL, l);
        return 8;
    case 0x77:
        Load8IntoMem(Reg16::HL, a);
        return 8;
    case 0x36:
        Load8IntoMem(Reg16::HL, GetImmediateByte());
        return 12;
    // LD A, (nn) -- Load value from memory at (nn) into A
    case 0x0A:
        Load8FromMem(Reg8::A, Read16(Reg16::BC));
        return 8;
    case 0x1A:
        Load8FromMem(Reg8::A, Read16(Reg16::DE));
        return 8;
    case 0xFA:
        Load8FromMem(Reg8::A, GetImmediateWord());
        return 16;
    // LD (nn), A -- Load value from A into memory at (nn)
    case 0x02:
        Load8IntoMem(Reg16::BC, a);
        return 8;
    case 0x12:
        Load8IntoMem(Reg16::DE, a);
        return 8;
    case 0xEA:
        LoadAIntoMem(GetImmediateWord());
        return 16;
    // LD (C), A -- Load value from A into memory at (0xFF00 + C)
    case 0xE2:
        LoadAIntoMem(0xFF00 + c);
        return 8;
    // LD A, (C) -- Load value from memory at (0xFF00 + C) into A
    case 0xF2:
        Load8FromMem(Reg8::A, 0xFF00 + c);
        return 8;
    // LDI (HL), A -- Load value from A into memory at (HL), then increment HL
    case 0x22:
        Load8IntoMem(Reg16::HL, a);
        IncHL();
        return 8;
    // LDI A, (HL) -- Load value from memory at (HL) into A, then increment HL
    case 0x2A:
        Load8FromMemAtHL(Reg8::A);
        IncHL();
        return 8;
    // LDD (HL), A -- Load value from A into memory at (HL), then decrement HL
    case 0x32:
        Load8IntoMem(Reg16::HL, a);
        DecHL();
        return 8;
    // LDD A, (HL) -- Load value from memory at (HL) into A, then decrement HL
    case 0x3A:
        Load8FromMemAtHL(Reg8::A);
        DecHL();
        return 8;
    // LDH (n), A -- Load value from A into memory at (0xFF00+n), with n as immediate byte value
    case 0xE0:
        LoadAIntoMem(0xFF00 + GetImmediateByte());
        return 12;
    // LDH A, (n) -- Load value from memory at (0xFF00+n) into A, with n as immediate byte value 
    case 0xF0:
        Load8FromMem(Reg8::A, 0xFF00 + GetImmediateByte());
        return 12;

    // ******** 16-bit loads ********
    // LD R, nn -- Load 16-bit immediate value into 16-bit register R
    case 0x01:
        Load16(Reg16::BC, GetImmediateWord());
        return 12;
    case 0x11:
        Load16(Reg16::DE, GetImmediateWord());
        return 12;
    case 0x21:
        Load16(Reg16::HL, GetImmediateWord());
        return 12;
    case 0x31:
        Load16(Reg16::SP, GetImmediateWord());
        return 12;
    // LD SP, HL -- Load value from HL into SP
    case 0xF9:
        LoadHLIntoSP();
        return 8;
    // LD HL, SP+n -- Load value from SP + n into HL, with n as signed immediate byte value
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Set appropriately, with immediate as unsigned byte.
    //     C: Set appropriately, with immediate as unsigned byte.
    case 0xF8:
        LoadSPnIntoHL(GetImmediateSignedByte());
        return 12;
    // LD (nn), SP -- Load value from SP into memory at (nn)
    case 0x08:
        LoadSPIntoMem(GetImmediateWord());
        return 20;
    // PUSH R -- Push 16-bit register R onto the stack and decrement the stack pointer by 2
    case 0xC5:
        Push(Reg16::BC);
        return 16;
    case 0xD5:
        Push(Reg16::DE);
        return 16;
    case 0xE5:
        Push(Reg16::HL);
        return 16;
    case 0xF5:
        Push(Reg16::AF);
        return 16;
    // POP R -- Pop 2 bytes off the stack into 16-bit register R and increment the stack pointer by 2
    case 0xC1:
        Pop(Reg16::BC);
        return 12;
    case 0xD1:
        Pop(Reg16::DE);
        return 12;
    case 0xE1:
        Pop(Reg16::HL);
        return 12;
    case 0xF1:
        Pop(Reg16::AF);
        return 12;

    // ******** 8-bit arithmetic and logic ********
    // ADD A, R -- Add value in register R to A
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Set if carry from bit 7
    case 0x80:
        Add(b);
        return 4;
    case 0x81:
        Add(c);
        return 4;
    case 0x82:
        Add(d);
        return 4;
    case 0x83:
        Add(e);
        return 4;
    case 0x84:
        Add(h);
        return 4;
    case 0x85:
        Add(l);
        return 4;
    case 0x86:
        AddFromMemAtHL();
        return 8;
    case 0x87:
        Add(a);
        return 4;
    // ADD A, n -- Add immediate value n to A
    // Flags: same as ADD A, R
    case 0xC6:
        Add(GetImmediateByte());
        return 4; //4; //cycles 8
    // ADC A, R -- Add value in register R + the carry flag to A
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Set if carry from bit 7
    case 0x88:
        AddWithCarry(b);
        return 4;
    case 0x89:
        AddWithCarry(c);
        return 4;
    case 0x8A:
        AddWithCarry(d);
        return 4;
    case 0x8B:
        AddWithCarry(e);
        return 4;
    case 0x8C:
        AddWithCarry(h);
        return 4;
    case 0x8D:
        AddWithCarry(l);
        return 4;
    case 0x8E:
        AddFromMemAtHLWithCarry();
        return 8;
    case 0x8F:
        AddWithCarry(a);
        return 4;
    // ADC A, n -- Add immediate value n + the carry flag to A
    // Flags: same as ADC A, R
    case 0xCE:
        AddWithCarry(GetImmediateByte());
        return 4; //4; //cycles 8
    // SUB R -- Subtract the value in register R from  A
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0x90:
        Sub(b);
        return 4;
    case 0x91:
        Sub(c);
        return 4;
    case 0x92:
        Sub(d);
        return 4;
    case 0x93:
        Sub(e);
        return 4;
    case 0x94:
        Sub(h);
        return 4;
    case 0x95:
        Sub(l);
        return 4;
    case 0x96:
        SubFromMemAtHL();
        return 8;
    case 0x97:
        Sub(a);
        return 4;
    // SUB n -- Subtract immediate value n from  A
    // Flags: same as SUB R
    case 0xD6:
        Sub(GetImmediateByte());
        return 4; //4; //cycles 8
    // SBC A, R -- Subtract the value in register R + carry flag from  A
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0x98:
        SubWithCarry(b);
        return 4;
    case 0x99:
        SubWithCarry(c);
        return 4;
    case 0x9A:
        SubWithCarry(d);
        return 4;
    case 0x9B:
        SubWithCarry(e);
        return 4;
    case 0x9C:
        SubWithCarry(h);
        return 4;
    case 0x9D:
        SubWithCarry(l);
        return 4;
    case 0x9E:
        SubFromMemAtHLWithCarry();
        return 8;
    case 0x9F:
        SubWithCarry(a);
        return 4;
    // SBC A, n -- Subtract immediate value n + carry flag from  A
    // Flags: same as SBC A, R
    case 0xDE:
        SubWithCarry(GetImmediateByte());
        return 4; //4; //cycles 8
    // AND R -- Bitwise AND the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set
    //     C: Reset
    case 0xA0:
        And(b);
        return 4;
    case 0xA1:
        And(c);
        return 4;
    case 0xA2:
        And(d);
        return 4;
    case 0xA3:
        And(e);
        return 4;
    case 0xA4:
        And(h);
        return 4;
    case 0xA5:
        And(l);
        return 4;
    case 0xA6:
        AndFromMemAtHL();
        return 8;
    case 0xA7:
        And(a);
        return 4;
    // AND n -- Bitwise AND the immediate value with A. 
    // Flags: same as AND R
    case 0xE6:
        And(GetImmediateByte());
        return 4; //4; //cycles 8
    // OR R -- Bitwise OR the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Reset
    //     C: Reset
    case 0xB0:
        Or(b);
        return 4;
    case 0xB1:
        Or(c);
        return 4;
    case 0xB2:
        Or(d);
        return 4;
    case 0xB3:
        Or(e);
        return 4;
    case 0xB4:
        Or(h);
        return 4;
    case 0xB5:
        Or(l);
        return 4;
    case 0xB6:
        OrFromMemAtHL();
        return 8;
    case 0xB7:
        Or(a);
        return 4;
    // OR n -- Bitwise OR the immediate value with A. 
    // Flags: same as OR R
    case 0xF6:
        Or(GetImmediateByte());
        return 4; //4; //cycles 8
    // XOR R -- Bitwise XOR the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Reset
    //     C: Reset
    case 0xA8:
        Xor(b);
        return 4;
    case 0xA9:
        Xor(c);
        return 4;
    case 0xAA:
        Xor(d);
        return 4;
    case 0xAB:
        Xor(e);
        return 4;
    case 0xAC:
        Xor(h);
        return 4;
    case 0xAD:
        Xor(l);
        return 4;
    case 0xAE:
        XorFromMemAtHL();
        return 8;
    case 0xAF:
        Xor(a);
        return 4;
    // XOR n -- Bitwise XOR the immediate value with A. 
    // Flags: same as XOR R
    case 0xEE:
        Xor(GetImmediateByte());
        return 4; //4; //cycles 8
    // CP R -- Compare A with the value in register R. This performs a subtraction but does not modify A.
    // Flags:
    //     Z: Set if result is zero, i.e. A is equal to R
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0xB8:
        Compare(b);
        return 4;
    case 0xB9:
        Compare(c);
        return 4;
    case 0xBA:
        Compare(d);
        return 4;
    case 0xBB:
        Compare(e);
        return 4;
    case 0xBC:
        Compare(h);
        return 4;
    case 0xBD:
        Compare(l);
        return 4;
    case 0xBE:
        CompareFromMemAtHL();
        return 8;
    case 0xBF:
        Compare(a);
        return 4;
    // CP n -- Compare A with the immediate value. This performs a subtraction but does not modify A.
    // Flags: same as CP R
    case 0xFE:
        Compare(GetImmediateByte());
        return 4; //4; //cycles 8
    // INC R -- Increment the value in register R.
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Unchanged
    case 0x04:
        IncReg(Reg8::B);
        return 4;
    case 0x0C:
        IncReg(Reg8::C);
        return 4;
    case 0x14:
        IncReg(Reg8::D);
        return 4;
    case 0x1C:
        IncReg(Reg8::E);
        return 4;
    case 0x24:
        IncReg(Reg8::H);
        return 4;
    case 0x2C:
        IncReg(Reg8::L);
        return 4;
    case 0x34:
        IncMemAtHL();
        return 12;
    case 0x3C:
        IncReg(Reg8::A);
        return 4;
    // DEC R -- Decrement the value in register R.
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Unchanged
    case 0x05:
        DecReg(Reg8::B);
        return 4;
    case 0x0D:
        DecReg(Reg8::C);
        return 4;
    case 0x15:
        DecReg(Reg8::D);
        return 4;
    case 0x1D:
        DecReg(Reg8::E);
        return 4;
    case 0x25:
        DecReg(Reg8::H);
        return 4;
    case 0x2D:
        DecReg(Reg8::L);
        return 4;
    case 0x35:
        DecMemAtHL();
        return 12;
    case 0x3D:
        DecReg(Reg8::A);
        return 4;

    // ******** 16-bit arithmetic ********
    // ADD HL, R -- Add the value in the 16-bit register R to HL.
    // Flags:
    //     Z: Unchanged
    //     N: Reset
    //     H: Set if carry from bit 11
    //     C: Set if carry from bit 15
    case 0x09:
        AddHL(Reg16::BC);
        return 8;
    case 0x19:
        AddHL(Reg16::DE);
        return 8;
    case 0x29:
        AddHL(Reg16::HL);
        return 8;
    case 0x39:
        AddHL(Reg16::SP);
        return 8;
    // ADD SP, n -- Add signed immediate byte to SP.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Set appropriately, with immediate as unsigned byte.
    //     C: Set appropriately, with immediate as unsigned byte.
    case 0xE8:
        AddSP(GetImmediateSignedByte());
        return 16;
    // INC R -- Increment the value in the 16-bit register R.
    // Flags unchanged
    case 0x03:
        IncReg(Reg16::BC);
        return 8;
    case 0x13:
        IncReg(Reg16::DE);
        return 8;
    case 0x23:
        IncReg(Reg16::HL);
        return 8;
    case 0x33:
        IncReg(Reg16::SP);
        return 8;
    // DEC R -- Decrement the value in the 16-bit register R.
    // Flags unchanged
    case 0x0B:
        DecReg(Reg16::BC);
        return 8;
    case 0x1B:
        DecReg(Reg16::DE);
        return 8;
    case 0x2B:
        DecReg(Reg16::HL);
        return 8;
    case 0x3B:
        DecReg(Reg16::SP);
        return 8;

    // ******** Miscellaneous Arithmetic ********
    // DAA -- Encode the contents of A in BCD.
    // Flags:
    //     Z: Set if result is zero
    //     N: Unchanged
    //     H: Reset
    //     C: Set appropriately
    case 0x27:
        DecimalAdjustA();
        return 4;
    // CPL -- Complement the value in register A.
    // Flags:
    //     Z: Unchanged
    //     N: Set
    //     H: Set
    //     C: Unchanged
    case 0x2F:
        ComplementA();
        return 4;
    // SCF -- Set the carry flag.
    // Flags:
    //     Z: Unchanged
    //     N: Reset
    //     H: Reset
    //     C: Set
    case 0x37:
        SetCarry();
        return 4;
    // CCF -- Complement the carry flag.
    // Flags:
    //     Z: Unchanged
    //     N: Reset
    //     H: Reset
    //     C: Complemented
    case 0x3F:
        ComplementCarry();
        return 4;

    // ******** Rotates and Shifts ********
    // RLCA -- Left rotate A.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 7 before the rotate
    case 0x07:
        RotateLeft(Reg8::A);
        f.SetZero(false);
        return 4;
    // RLA -- Left rotate A through the carry flag.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 7 before the rotate
    case 0x17:
        RotateLeftThroughCarry(Reg8::A);
        f.SetZero(false);
        return 4;
    // RRCA -- Right rotate A.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 0 before the rotate
    case 0x0F:
        RotateRight(Reg8::A);
        f.SetZero(false);
        return 4;
    // RRA -- Right rotate A through the carry flag.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 0 before the rotate
    case 0x1F:
        RotateRightThroughCarry(Reg8::A);
        f.SetZero(false);
        return 4;

    // ******** Jumps ********
    // JP nn -- Jump to the address given by the 16-bit immediate value.
    case 0xC3:
        Jump(GetImmediateWord());
        return 16;
    // JP cc, nn -- Jump to the address given by the 16-bit immediate value if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0xC2:
        if (!f.Zero()) {
            Jump(GetImmediateWord());
            return 16;
        } else {
            HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xCA:
        if (f.Zero()) {
            Jump(GetImmediateWord());
            return 16;
        } else {
            HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xD2:
        if (!f.Carry()) {
            Jump(GetImmediateWord());
            return 16;
        } else {
            HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xDA:
        if (f.Carry()) {
            Jump(GetImmediateWord());
            return 16;
        } else {
            HardwareTick(8);
            pc += 2;
            return 12;
        }
    // JP (HL) -- Jump to the address contained in HL.
    case 0xE9:
        JumpToHL();
        return 4;
    // JR n -- Jump to the current address + immediate signed byte.
    case 0x18:
        RelativeJump(GetImmediateSignedByte());
        return 12;
    // JR cc, n -- Jump to the current address + immediate signed byte if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0x20:
        if (!f.Zero()) {
            RelativeJump(GetImmediateSignedByte());
            return 12;
        } else {
            HardwareTick(4);
            ++pc;
            return 8;
        }
    case 0x28:
        if (f.Zero()) {
            RelativeJump(GetImmediateSignedByte());
            return 12;
        } else {
            HardwareTick(4);
            ++pc;
            return 8;
        }
    case 0x30:
        if (!f.Carry()) {
            RelativeJump(GetImmediateSignedByte());
            return 12;
        } else {
            HardwareTick(4);
            ++pc;
            return 8;
        }
    case 0x38:
        if (f.Carry()) {
            RelativeJump(GetImmediateSignedByte());
            return 12;
        } else {
            HardwareTick(4);
            ++pc;
            return 8;
        }

    // ******** Calls ********
    // CALL nn -- Push address of the next instruction onto the stack, and jump to the address given by 
    // the 16-bit immediate value.
    case 0xCD:
        Call(GetImmediateWord());
        return 24;
    // CALL cc, nn -- Push address of the next instruction onto the stack, and jump to the address given by 
    // the 16-bit immediate value, if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0xC4:
        if (!f.Zero()) {
            Call(GetImmediateWord());
            return 24;
        } else {
            HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xCC:
        if (f.Zero()) {
            Call(GetImmediateWord());
            return 24;
        } else {
            HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xD4:
        if (!f.Carry()) {
            Call(GetImmediateWord());
            return 24;
        } else {
            HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xDC:
        if (f.Carry()) {
            Call(GetImmediateWord());
            return 24;
        } else {
            HardwareTick(8);
            pc += 2;
            return 12;
        }

    // ******** Returns ********
    // RET -- Pop two bytes off the stack and jump to their effective address.
    case 0xC9:
        Return();
        return 16;
    // RET cc -- Pop two bytes off the stack and jump to their effective address, if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0xC0:
        HardwareTick(4); // For the comparison.
        if (!f.Zero()) {
            Return();
            return 20;
        } else {
            return 8;
        }
    case 0xC8:
        HardwareTick(4); // For the comparison.
        if (f.Zero()) {
            Return();
            return 20;
        } else {
            return 8;
        }
    case 0xD0:
        HardwareTick(4); // For the comparison.
        if (!f.Carry()) {
            Return();
            return 20;
        } else {
            return 8;
        }
    case 0xD8:
        HardwareTick(4); // For the comparison.
        if (f.Carry()) {
            Return();
            return 20;
        } else {
            return 8;
        }
    // RETI -- Pop two bytes off the stack and jump to their effective address, and enable interrupts.
    case 0xD9:
        Return();
        interrupt_master_enable = true;
        return 16;

    // ******** Restarts ********
    // RST n -- Push address of next instruction onto the stack, and jump to the
    // address given by n.
    case 0xC7:
        Call(0x0000);
        return 16;
    case 0xCF:
        Call(0x0008);
        return 16;
    case 0xD7:
        Call(0x0010);
        return 16;
    case 0xDF:
        Call(0x0018);
        return 16;
    case 0xE7:
        Call(0x0020);
        return 16;
    case 0xEF:
        Call(0x0028);
        return 16;
    case 0xF7:
        Call(0x0030);
        return 16;
    case 0xFF:
        Call(0x0038);
        return 16;

    // ******** System Control ********
    // NOP -- No operation.
    case 0x00:
        return 4;
    // HALT -- Put CPU into lower power mode until an interrupt occurs.
    case 0x76:
        Halt();
        return 4;
    // STOP -- Halt both the CPU and LCD until a button is pressed. Can also be used to switch to double-speed mode.
    case 0x10:
        Stop();
        return 4;
    // DI -- Disable interrupts.
    case 0xF3:
        interrupt_master_enable = false;
        return 4;
    // EI -- Enable interrupts after the next instruction is executed.
    case 0xFB:
        enable_interrupts_delayed = true;
        return 4;

    // ******** CB prefix opcodes ********
    case 0xCB:
        // Get opcode suffix from next byte.
        switch (GetImmediateByte()) {
        // ******** Rotates and Shifts ********
        // RLC R -- Left rotate the value in register R.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 7 before the rotate
        case 0x00:
            RotateLeft(Reg8::B);
            return 8;
        case 0x01:
            RotateLeft(Reg8::C);
            return 8;
        case 0x02:
            RotateLeft(Reg8::D);
            return 8;
        case 0x03:
            RotateLeft(Reg8::E);
            return 8;
        case 0x04:
            RotateLeft(Reg8::H);
            return 8;
        case 0x05:
            RotateLeft(Reg8::L);
            return 8;
        case 0x06:
            RotateLeftMemAtHL();
            return 16;
        case 0x07:
            RotateLeft(Reg8::A);
            return 8;
        // RL R -- Left rotate the value in register R through the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 7 before the rotate
        case 0x10:
            RotateLeftThroughCarry(Reg8::B);
            return 8;
        case 0x11:
            RotateLeftThroughCarry(Reg8::C);
            return 8;
        case 0x12:
            RotateLeftThroughCarry(Reg8::D);
            return 8;
        case 0x13:
            RotateLeftThroughCarry(Reg8::E);
            return 8;
        case 0x14:
            RotateLeftThroughCarry(Reg8::H);
            return 8;
        case 0x15:
            RotateLeftThroughCarry(Reg8::L);
            return 8;
        case 0x16:
            RotateLeftMemAtHLThroughCarry();
            return 16;
        case 0x17:
            RotateLeftThroughCarry(Reg8::A);
            return 8;
        // RRC R -- Right rotate the value in register R.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x08:
            RotateRight(Reg8::B);
            return 8;
        case 0x09:
            RotateRight(Reg8::C);
            return 8;
        case 0x0A:
            RotateRight(Reg8::D);
            return 8;
        case 0x0B:
            RotateRight(Reg8::E);
            return 8;
        case 0x0C:
            RotateRight(Reg8::H);
            return 8;
        case 0x0D:
            RotateRight(Reg8::L);
            return 8;
        case 0x0E:
            RotateRightMemAtHL();
            return 16;
        case 0x0F:
            RotateRight(Reg8::A);
            return 8;
        // RR R -- Right rotate the value in register R through the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x18:
            RotateRightThroughCarry(Reg8::B);
            return 8;
        case 0x19:
            RotateRightThroughCarry(Reg8::C);
            return 8;
        case 0x1A:
            RotateRightThroughCarry(Reg8::D);
            return 8;
        case 0x1B:
            RotateRightThroughCarry(Reg8::E);
            return 8;
        case 0x1C:
            RotateRightThroughCarry(Reg8::H);
            return 8;
        case 0x1D:
            RotateRightThroughCarry(Reg8::L);
            return 8;
        case 0x1E:
            RotateRightMemAtHLThroughCarry();
            return 16;
        case 0x1F:
            RotateRightThroughCarry(Reg8::A);
            return 8;
        // SLA R -- Left shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x20:
            ShiftLeft(Reg8::B);
            return 8;
        case 0x21:
            ShiftLeft(Reg8::C);
            return 8;
        case 0x22:
            ShiftLeft(Reg8::D);
            return 8;
        case 0x23:
            ShiftLeft(Reg8::E);
            return 8;
        case 0x24:
            ShiftLeft(Reg8::H);
            return 8;
        case 0x25:
            ShiftLeft(Reg8::L);
            return 8;
        case 0x26:
            ShiftLeftMemAtHL();
            return 16;
        case 0x27:
            ShiftLeft(Reg8::A);
            return 8;
        // SRA R -- Arithmetic right shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x28:
            ShiftRightArithmetic(Reg8::B);
            return 8;
        case 0x29:
            ShiftRightArithmetic(Reg8::C);
            return 8;
        case 0x2A:
            ShiftRightArithmetic(Reg8::D);
            return 8;
        case 0x2B:
            ShiftRightArithmetic(Reg8::E);
            return 8;
        case 0x2C:
            ShiftRightArithmetic(Reg8::H);
            return 8;
        case 0x2D:
            ShiftRightArithmetic(Reg8::L);
            return 8;
        case 0x2E:
            ShiftRightArithmeticMemAtHL();
            return 16;
        case 0x2F:
            ShiftRightArithmetic(Reg8::A);
            return 8;
        // SWAP R -- Swap upper and lower nybbles of register R (rotate by 4).
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Reset
        case 0x30:
            SwapNybbles(Reg8::B);
            return 8;
        case 0x31:
            SwapNybbles(Reg8::C);
            return 8;
        case 0x32:
            SwapNybbles(Reg8::D);
            return 8;
        case 0x33:
            SwapNybbles(Reg8::E);
            return 8;
        case 0x34:
            SwapNybbles(Reg8::H);
            return 8;
        case 0x35:
            SwapNybbles(Reg8::L);
            return 8;
        case 0x36:
            SwapMemAtHL();
            return 16;
        case 0x37:
            SwapNybbles(Reg8::A);
            return 8;
        // SRL R -- Logical right shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x38:
            ShiftRightLogical(Reg8::B);
            return 8;
        case 0x39:
            ShiftRightLogical(Reg8::C);
            return 8;
        case 0x3A:
            ShiftRightLogical(Reg8::D);
            return 8;
        case 0x3B:
            ShiftRightLogical(Reg8::E);
            return 8;
        case 0x3C:
            ShiftRightLogical(Reg8::H);
            return 8;
        case 0x3D:
            ShiftRightLogical(Reg8::L);
            return 8;
        case 0x3E:
            ShiftRightLogicalMemAtHL();
            return 16;
        case 0x3F:
            ShiftRightLogical(Reg8::A);
            return 8;

        // ******** Bit Manipulation ********
        // BIT b, R -- test bit b of the value in register R.
        // Flags:
        //     Z: Set if bit b of R is zero
        //     N: Reset
        //     H: Set
        //     C: Unchanged
        case 0x40:
            TestBit(0, b);
            return 8;
        case 0x41:
            TestBit(0, c);
            return 8;
        case 0x42:
            TestBit(0, d);
            return 8;
        case 0x43:
            TestBit(0, e);
            return 8;
        case 0x44:
            TestBit(0, h);
            return 8;
        case 0x45:
            TestBit(0, l);
            return 8;
        case 0x46:
            TestBitOfMemAtHL(0);
            return 12;
        case 0x47:
            TestBit(0, a);
            return 8;
        case 0x48:
            TestBit(1, b);
            return 8;
        case 0x49:
            TestBit(1, c);
            return 8;
        case 0x4A:
            TestBit(1, d);
            return 8;
        case 0x4B:
            TestBit(1, e);
            return 8;
        case 0x4C:
            TestBit(1, h);
            return 8;
        case 0x4D:
            TestBit(1, l);
            return 8;
        case 0x4E:
            TestBitOfMemAtHL(1);
            return 12;
        case 0x4F:
            TestBit(1, a);
            return 8;
        case 0x50:
            TestBit(2, b);
            return 8;
        case 0x51:
            TestBit(2, c);
            return 8;
        case 0x52:
            TestBit(2, d);
            return 8;
        case 0x53:
            TestBit(2, e);
            return 8;
        case 0x54:
            TestBit(2, h);
            return 8;
        case 0x55:
            TestBit(2, l);
            return 8;
        case 0x56:
            TestBitOfMemAtHL(2);
            return 12;
        case 0x57:
            TestBit(2, a);
            return 8;
        case 0x58:
            TestBit(3, b);
            return 8;
        case 0x59:
            TestBit(3, c);
            return 8;
        case 0x5A:
            TestBit(3, d);
            return 8;
        case 0x5B:
            TestBit(3, e);
            return 8;
        case 0x5C:
            TestBit(3, h);
            return 8;
        case 0x5D:
            TestBit(3, l);
            return 8;
        case 0x5E:
            TestBitOfMemAtHL(3);
            return 12;
        case 0x5F:
            TestBit(3, a);
            return 8;
        case 0x60:
            TestBit(4, b);
            return 8;
        case 0x61:
            TestBit(4, c);
            return 8;
        case 0x62:
            TestBit(4, d);
            return 8;
        case 0x63:
            TestBit(4, e);
            return 8;
        case 0x64:
            TestBit(4, h);
            return 8;
        case 0x65:
            TestBit(4, l);
            return 8;
        case 0x66:
            TestBitOfMemAtHL(4);
            return 12;
        case 0x67:
            TestBit(4, a);
            return 8;
        case 0x68:
            TestBit(5, b);
            return 8;
        case 0x69:
            TestBit(5, c);
            return 8;
        case 0x6A:
            TestBit(5, d);
            return 8;
        case 0x6B:
            TestBit(5, e);
            return 8;
        case 0x6C:
            TestBit(5, h);
            return 8;
        case 0x6D:
            TestBit(5, l);
            return 8;
        case 0x6E:
            TestBitOfMemAtHL(5);
            return 12;
        case 0x6F:
            TestBit(5, a);
            return 8;
        case 0x70:
            TestBit(6, b);
            return 8;
        case 0x71:
            TestBit(6, c);
            return 8;
        case 0x72:
            TestBit(6, d);
            return 8;
        case 0x73:
            TestBit(6, e);
            return 8;
        case 0x74:
            TestBit(6, h);
            return 8;
        case 0x75:
            TestBit(6, l);
            return 8;
        case 0x76:
            TestBitOfMemAtHL(6);
            return 12;
        case 0x77:
            TestBit(6, a);
            return 8;
        case 0x78:
            TestBit(7, b);
            return 8;
        case 0x79:
            TestBit(7, c);
            return 8;
        case 0x7A:
            TestBit(7, d);
            return 8;
        case 0x7B:
            TestBit(7, e);
            return 8;
        case 0x7C:
            TestBit(7, h);
            return 8;
        case 0x7D:
            TestBit(7, l);
            return 8;
        case 0x7E:
            TestBitOfMemAtHL(7);
            return 12;
        case 0x7F:
            TestBit(7, a);
            return 8;
        // RES b, R -- reset bit b of the value in register R.
        // Flags unchanged
        case 0x80:
            ResetBit(0, Reg8::B);
            return 8;
        case 0x81:
            ResetBit(0, Reg8::C);
            return 8;
        case 0x82:
            ResetBit(0, Reg8::D);
            return 8;
        case 0x83:
            ResetBit(0, Reg8::E);
            return 8;
        case 0x84:
            ResetBit(0, Reg8::H);
            return 8;
        case 0x85:
            ResetBit(0, Reg8::L);
            return 8;
        case 0x86:
            ResetBitOfMemAtHL(0);
            return 16;
        case 0x87:
            ResetBit(0, Reg8::A);
            return 8;
        case 0x88:
            ResetBit(1, Reg8::B);
            return 8;
        case 0x89:
            ResetBit(1, Reg8::C);
            return 8;
        case 0x8A:
            ResetBit(1, Reg8::D);
            return 8;
        case 0x8B:
            ResetBit(1, Reg8::E);
            return 8;
        case 0x8C:
            ResetBit(1, Reg8::H);
            return 8;
        case 0x8D:
            ResetBit(1, Reg8::L);
            return 8;
        case 0x8E:
            ResetBitOfMemAtHL(1);
            return 16;
        case 0x8F:
            ResetBit(1, Reg8::A);
            return 8;
        case 0x90:
            ResetBit(2, Reg8::B);
            return 8;
        case 0x91:
            ResetBit(2, Reg8::C);
            return 8;
        case 0x92:
            ResetBit(2, Reg8::D);
            return 8;
        case 0x93:
            ResetBit(2, Reg8::E);
            return 8;
        case 0x94:
            ResetBit(2, Reg8::H);
            return 8;
        case 0x95:
            ResetBit(2, Reg8::L);
            return 8;
        case 0x96:
            ResetBitOfMemAtHL(2);
            return 16;
        case 0x97:
            ResetBit(2, Reg8::A);
            return 8;
        case 0x98:
            ResetBit(3, Reg8::B);
            return 8;
        case 0x99:
            ResetBit(3, Reg8::C);
            return 8;
        case 0x9A:
            ResetBit(3, Reg8::D);
            return 8;
        case 0x9B:
            ResetBit(3, Reg8::E);
            return 8;
        case 0x9C:
            ResetBit(3, Reg8::H);
            return 8;
        case 0x9D:
            ResetBit(3, Reg8::L);
            return 8;
        case 0x9E:
            ResetBitOfMemAtHL(3);
            return 16;
        case 0x9F:
            ResetBit(3, Reg8::A);
            return 8;
        case 0xA0:
            ResetBit(4, Reg8::B);
            return 8;
        case 0xA1:
            ResetBit(4, Reg8::C);
            return 8;
        case 0xA2:
            ResetBit(4, Reg8::D);
            return 8;
        case 0xA3:
            ResetBit(4, Reg8::E);
            return 8;
        case 0xA4:
            ResetBit(4, Reg8::H);
            return 8;
        case 0xA5:
            ResetBit(4, Reg8::L);
            return 8;
        case 0xA6:
            ResetBitOfMemAtHL(4);
            return 16;
        case 0xA7:
            ResetBit(4, Reg8::A);
            return 8;
        case 0xA8:
            ResetBit(5, Reg8::B);
            return 8;
        case 0xA9:
            ResetBit(5, Reg8::C);
            return 8;
        case 0xAA:
            ResetBit(5, Reg8::D);
            return 8;
        case 0xAB:
            ResetBit(5, Reg8::E);
            return 8;
        case 0xAC:
            ResetBit(5, Reg8::H);
            return 8;
        case 0xAD:
            ResetBit(5, Reg8::L);
            return 8;
        case 0xAE:
            ResetBitOfMemAtHL(5);
            return 16;
        case 0xAF:
            ResetBit(5, Reg8::A);
            return 8;
        case 0xB0:
            ResetBit(6, Reg8::B);
            return 8;
        case 0xB1:
            ResetBit(6, Reg8::C);
            return 8;
        case 0xB2:
            ResetBit(6, Reg8::D);
            return 8;
        case 0xB3:
            ResetBit(6, Reg8::E);
            return 8;
        case 0xB4:
            ResetBit(6, Reg8::H);
            return 8;
        case 0xB5:
            ResetBit(6, Reg8::L);
            return 8;
        case 0xB6:
            ResetBitOfMemAtHL(6);
            return 16;
        case 0xB7:
            ResetBit(6, Reg8::A);
            return 8;
        case 0xB8:
            ResetBit(7, Reg8::B);
            return 8;
        case 0xB9:
            ResetBit(7, Reg8::C);
            return 8;
        case 0xBA:
            ResetBit(7, Reg8::D);
            return 8;
        case 0xBB:
            ResetBit(7, Reg8::E);
            return 8;
        case 0xBC:
            ResetBit(7, Reg8::H);
            return 8;
        case 0xBD:
            ResetBit(7, Reg8::L);
            return 8;
        case 0xBE:
            ResetBitOfMemAtHL(7);
            return 16;
        case 0xBF:
            ResetBit(7, Reg8::A);
            return 8;
        // SET b, R -- set bit b of the value in register R.
        // Flags unchanged
        case 0xC0:
            SetBit(0, Reg8::B);
            return 8;
        case 0xC1:
            SetBit(0, Reg8::C);
            return 8;
        case 0xC2:
            SetBit(0, Reg8::D);
            return 8;
        case 0xC3:
            SetBit(0, Reg8::E);
            return 8;
        case 0xC4:
            SetBit(0, Reg8::H);
            return 8;
        case 0xC5:
            SetBit(0, Reg8::L);
            return 8;
        case 0xC6:
            SetBitOfMemAtHL(0);
            return 16;
        case 0xC7:
            SetBit(0, Reg8::A);
            return 8;
        case 0xC8:
            SetBit(1, Reg8::B);
            return 8;
        case 0xC9:
            SetBit(1, Reg8::C);
            return 8;
        case 0xCA:
            SetBit(1, Reg8::D);
            return 8;
        case 0xCB:
            SetBit(1, Reg8::E);
            return 8;
        case 0xCC:
            SetBit(1, Reg8::H);
            return 8;
        case 0xCD:
            SetBit(1, Reg8::L);
            return 8;
        case 0xCE:
            SetBitOfMemAtHL(1);
            return 16;
        case 0xCF:
            SetBit(1, Reg8::A);
            return 8;
        case 0xD0:
            SetBit(2, Reg8::B);
            return 8;
        case 0xD1:
            SetBit(2, Reg8::C);
            return 8;
        case 0xD2:
            SetBit(2, Reg8::D);
            return 8;
        case 0xD3:
            SetBit(2, Reg8::E);
            return 8;
        case 0xD4:
            SetBit(2, Reg8::H);
            return 8;
        case 0xD5:
            SetBit(2, Reg8::L);
            return 8;
        case 0xD6:
            SetBitOfMemAtHL(2);
            return 16;
        case 0xD7:
            SetBit(2, Reg8::A);
            return 8;
        case 0xD8:
            SetBit(3, Reg8::B);
            return 8;
        case 0xD9:
            SetBit(3, Reg8::C);
            return 8;
        case 0xDA:
            SetBit(3, Reg8::D);
            return 8;
        case 0xDB:
            SetBit(3, Reg8::E);
            return 8;
        case 0xDC:
            SetBit(3, Reg8::H);
            return 8;
        case 0xDD:
            SetBit(3, Reg8::L);
            return 8;
        case 0xDE:
            SetBitOfMemAtHL(3);
            return 16;
        case 0xDF:
            SetBit(3, Reg8::A);
            return 8;
        case 0xE0:
            SetBit(4, Reg8::B);
            return 8;
        case 0xE1:
            SetBit(4, Reg8::C);
            return 8;
        case 0xE2:
            SetBit(4, Reg8::D);
            return 8;
        case 0xE3:
            SetBit(4, Reg8::E);
            return 8;
        case 0xE4:
            SetBit(4, Reg8::H);
            return 8;
        case 0xE5:
            SetBit(4, Reg8::L);
            return 8;
        case 0xE6:
            SetBitOfMemAtHL(4);
            return 16;
        case 0xE7:
            SetBit(4, Reg8::A);
            return 8;
        case 0xE8:
            SetBit(5, Reg8::B);
            return 8;
        case 0xE9:
            SetBit(5, Reg8::C);
            return 8;
        case 0xEA:
            SetBit(5, Reg8::D);
            return 8;
        case 0xEB:
            SetBit(5, Reg8::E);
            return 8;
        case 0xEC:
            SetBit(5, Reg8::H);
            return 8;
        case 0xED:
            SetBit(5, Reg8::L);
            return 8;
        case 0xEE:
            SetBitOfMemAtHL(5);
            return 16;
        case 0xEF:
            SetBit(5, Reg8::A);
            return 8;
        case 0xF0:
            SetBit(6, Reg8::B);
            return 8;
        case 0xF1:
            SetBit(6, Reg8::C);
            return 8;
        case 0xF2:
            SetBit(6, Reg8::D);
            return 8;
        case 0xF3:
            SetBit(6, Reg8::E);
            return 8;
        case 0xF4:
            SetBit(6, Reg8::H);
            return 8;
        case 0xF5:
            SetBit(6, Reg8::L);
            return 8;
        case 0xF6:
            SetBitOfMemAtHL(6);
            return 16;
        case 0xF7:
            SetBit(6, Reg8::A);
            return 8;
        case 0xF8:
            SetBit(7, Reg8::B);
            return 8;
        case 0xF9:
            SetBit(7, Reg8::C);
            return 8;
        case 0xFA:
            SetBit(7, Reg8::D);
            return 8;
        case 0xFB:
            SetBit(7, Reg8::E);
            return 8;
        case 0xFC:
            SetBit(7, Reg8::H);
            return 8;
        case 0xFD:
            SetBit(7, Reg8::L);
            return 8;
        case 0xFE:
            SetBitOfMemAtHL(7);
            return 16;
        case 0xFF:
            SetBit(7, Reg8::A);
            return 8;

        default:
            // Unreachable, every possible case is specified above.
            return 8;
        }

    default:
        return 4;
    }
}

} // End namespace Core
