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

#include <stdexcept>

#include "gb/cpu/Cpu.h"
#include "gb/memory/Memory.h"
#include "gb/core/GameBoy.h"
#include "gb/logging/Logging.h"

namespace Gb {

CPU::CPU(Memory& _mem, GameBoy& _gameboy)
        : mem(_mem)
        , gameboy(_gameboy) {
    // Initial register values
    if (gameboy.GameModeDmg()) {
        if (gameboy.console == Console::DMG) {
            regs.reg16[AF] = 0x01B0;
            regs.reg16[BC] = 0x0013;
            regs.reg16[DE] = 0x00D8;
            regs.reg16[HL] = 0x014D;
        } else if (gameboy.console == Console::CGB) {
            regs.reg16[AF] = 0x1180;
            regs.reg16[BC] = 0x0000;
            regs.reg16[DE] = 0x0008;
            regs.reg16[HL] = 0x007C;
        } else if (gameboy.console == Console::AGB) {
            regs.reg16[AF] = 0x1100;
            regs.reg16[BC] = 0x0100;
            regs.reg16[DE] = 0x0008;
            regs.reg16[HL] = 0x007C;
        }
    } else if (gameboy.GameModeCgb()) {
        if (gameboy.console == Console::CGB) {
            regs.reg16[AF] = 0x1180;
            regs.reg16[BC] = 0x0000;
            regs.reg16[DE] = 0xFF56;
            regs.reg16[HL] = 0x000D;
        } else if (gameboy.console == Console::AGB) {
            regs.reg16[AF] = 0x1100;
            regs.reg16[BC] = 0x0100;
            regs.reg16[DE] = 0xFF56;
            regs.reg16[HL] = 0x000D;
        }
    }

    regs.reg16[SP] = 0xFFFE;
}

u8 CPU::ReadMemAndTick(const u16 addr) {
    const u8 data = mem.ReadMem(addr);
    gameboy.HardwareTick(4);
    return data;
}

void CPU::WriteMemAndTick(const u16 addr, const u8 val) {
    mem.WriteMem(addr, val);
    gameboy.HardwareTick(4);
}

u8 CPU::GetImmediateByte() {
    return ReadMemAndTick(pc++);
}

u16 CPU::GetImmediateWord() {
    const u8 byte_lo = ReadMemAndTick(pc++);
    const u8 byte_hi = ReadMemAndTick(pc++);

    return (static_cast<u16>(byte_hi) << 8) | static_cast<u16>(byte_lo);
}

int CPU::RunFor(int cycles) {
    // Execute instructions until the specified number of cycles has passed.
    while (cycles > 0) {
        if (cpu_mode == CPUMode::Stopped) {
            StoppedTick();
            cycles -= 4;
            continue;
        } else if (mem.HDMAInProgress() && cpu_mode != CPUMode::Halted) {
            mem.UpdateHDMA();
            gameboy.HaltedTick(4);
            cycles -= 4;
            continue;
        }

        cycles -= HandleInterrupts();

        if (cpu_mode == CPUMode::Running) {
            gameboy.logging->LogInstruction(regs, pc);
            cycles -= ExecuteNext(mem.ReadMem(pc++));
        } else if (cpu_mode == CPUMode::HaltBug) {
            gameboy.logging->LogInstruction(regs, pc);
            cycles -= ExecuteNext(mem.ReadMem(pc));
            cpu_mode = CPUMode::Running;
        } else if (cpu_mode == CPUMode::Halted) {
            gameboy.HaltedTick(4);
            gameboy.logging->IncHaltCycles(4);
            cycles -= 4;
        }
    }

    // Return the number of overspent cycles.
    return cycles;
}

int CPU::HandleInterrupts() {
    if (interrupt_master_enable) {
        if (mem.RequestedEnabledInterrupts()) {
            gameboy.logging->LogInterrupt();

            // Disable interrupts.
            interrupt_master_enable = false;

            // The Game Boy reads IF & IE once to check for pending interrupts. Then it pushes the high byte of PC
            // and waits a total of 4 M-cycles before it reads IF & IE again to see which interrupt to service. As
            // a result, if a higher priority interrupt occurs before the second IF read, it will be serviced instead
            // of the one that triggered the interrupt handler.
            gameboy.HardwareTick(8);
            WriteMemAndTick(--regs.reg16[SP], static_cast<u8>(pc >> 8));
            gameboy.HardwareTick(4);

            u16 interrupt_vector = 0x0000;
            if (mem.IsPending(Interrupt::VBlank)) {
                mem.ClearInterrupt(Interrupt::VBlank);
                interrupt_vector = 0x0040;
            } else if (mem.IsPending(Interrupt::Stat)) {
                mem.ClearInterrupt(Interrupt::Stat);
                interrupt_vector = 0x0048;
            } else if (mem.IsPending(Interrupt::Timer)) {
                mem.ClearInterrupt(Interrupt::Timer);
                interrupt_vector = 0x0050;
            } else if (mem.IsPending(Interrupt::Serial)) {
                mem.ClearInterrupt(Interrupt::Serial);
                interrupt_vector = 0x0058;
            } else if (mem.IsPending(Interrupt::Joypad)) {
                mem.ClearInterrupt(Interrupt::Joypad);
                interrupt_vector = 0x0060;
            }

            WriteMemAndTick(--regs.reg16[SP], static_cast<u8>(pc));
            pc = interrupt_vector;

            if (cpu_mode == CPUMode::Halted) {
                // Exit halt mode.
                cpu_mode = CPUMode::Running;
                gameboy.logging->LogHalt();
            }

            return 20;
        }
    } else if (cpu_mode == CPUMode::Halted) {
        if (mem.RequestedEnabledInterrupts()) {
            // If halt mode is entered when IME is zero, then the next time an interrupt is triggered the CPU does 
            // not jump to the interrupt routine or clear the IF flag. It just exits halt mode and continues execution.
            cpu_mode = CPUMode::Running;
            gameboy.logging->LogHalt();
        }
    }

    return 0;
}

void CPU::EnableInterruptsDelayed() {
    interrupt_master_enable = interrupt_master_enable || enable_interrupts_delayed;
    enable_interrupts_delayed = false;
}

void CPU::StoppedTick() {
    gameboy.HaltedTick(4);

    if (gameboy.JoypadPress()) {
        if (speed_switch_cycles != 0) {
            // The CPU hangs if there is an enabled joypad press during a speed switch.
            throw std::runtime_error("The CPU has hung. Reason: enabled joypad press during a speed switch.");
        } else {
            // Exit STOP mode.
            cpu_mode = CPUMode::Running;
        }
    }

    // speed_switch_cycles is 0 if we're just in regular stop mode.
    if (speed_switch_cycles > 0) {
        if (speed_switch_cycles == 4) {
            // Speed switch finished.
            gameboy.SpeedSwitch();

            // Exit STOP mode.
            cpu_mode = CPUMode::Running;
        }

        speed_switch_cycles -= 4;
    }
}

unsigned int CPU::ExecuteNext(const u8 opcode) {
    gameboy.HardwareTick(4);

    switch (opcode) {
    // ******** 8-bit loads ********
    // LD R, n -- Load immediate value n into register R
    case 0x06:
        Load8Immediate(B, GetImmediateByte());
        return 8;
    case 0x0E:
        Load8Immediate(C, GetImmediateByte());
        return 8;
    case 0x16:
        Load8Immediate(D, GetImmediateByte());
        return 8;
    case 0x1E:
        Load8Immediate(E, GetImmediateByte());
        return 8;
    case 0x26:
        Load8Immediate(H, GetImmediateByte());
        return 8;
    case 0x2E:
        Load8Immediate(L, GetImmediateByte());
        return 8;
    case 0x3E:
        Load8Immediate(A, GetImmediateByte());
        return 8;
    // LD A, R2 -- Load value from R2 into A
    case 0x78:
        Load8(A, B);
        return 4;
    case 0x79:
        Load8(A, C);
        return 4;
    case 0x7A:
        Load8(A, D);
        return 4;
    case 0x7B:
        Load8(A, E);
        return 4;
    case 0x7C:
        Load8(A, H);
        return 4;
    case 0x7D:
        Load8(A, L);
        return 4;
    case 0x7E:
        Load8FromMem(A, regs.reg16[HL]);
        return 8;
    case 0x7F:
        Load8(A, A);
        return 4;
    // LD B, R2 -- Load value from R2 into B
    case 0x40:
        Load8(B, B);
        return 4;
    case 0x41:
        Load8(B, C);
        return 4;
    case 0x42:
        Load8(B, D);
        return 4;
    case 0x43:
        Load8(B, E);
        return 4;
    case 0x44:
        Load8(B, H);
        return 4;
    case 0x45:
        Load8(B, L);
        return 4;
    case 0x46:
        Load8FromMem(B, regs.reg16[HL]);
        return 8;
    case 0x47:
        Load8(B, A);
        return 4;
    // LD C, R2 -- Load value from R2 into C
    case 0x48:
        Load8(C, B);
        return 4;
    case 0x49:
        Load8(C, C);
        return 4;
    case 0x4A:
        Load8(C, D);
        return 4;
    case 0x4B:
        Load8(C, E);
        return 4;
    case 0x4C:
        Load8(C, H);
        return 4;
    case 0x4D:
        Load8(C, L);
        return 4;
    case 0x4E:
        Load8FromMem(C, regs.reg16[HL]);
        return 8;
    case 0x4F:
        Load8(C, A);
        return 4;
    // LD D, R2 -- Load value from R2 into D
    case 0x50:
        Load8(D, B);
        return 4;
    case 0x51:
        Load8(D, C);
        return 4;
    case 0x52:
        Load8(D, D);
        return 4;
    case 0x53:
        Load8(D, E);
        return 4;
    case 0x54:
        Load8(D, H);
        return 4;
    case 0x55:
        Load8(D, L);
        return 4;
    case 0x56:
        Load8FromMem(D, regs.reg16[HL]);
        return 8;
    case 0x57:
        Load8(D, A);
        return 4;
    // LD E, R2 -- Load value from R2 into E
    case 0x58:
        Load8(E, B);
        return 4;
    case 0x59:
        Load8(E, C);
        return 4;
    case 0x5A:
        Load8(E, D);
        return 4;
    case 0x5B:
        Load8(E, E);
        return 4;
    case 0x5C:
        Load8(E, H);
        return 4;
    case 0x5D:
        Load8(E, L);
        return 4;
    case 0x5E:
        Load8FromMem(E, regs.reg16[HL]);
        return 8;
    case 0x5F:
        Load8(E, A);
        return 4;
    // LD H, R2 -- Load value from R2 into H
    case 0x60:
        Load8(H, B);
        return 4;
    case 0x61:
        Load8(H, C);
        return 4;
    case 0x62:
        Load8(H, D);
        return 4;
    case 0x63:
        Load8(H, E);
        return 4;
    case 0x64:
        Load8(H, H);
        return 4;
    case 0x65:
        Load8(H, L);
        return 4;
    case 0x66:
        Load8FromMem(H, regs.reg16[HL]);
        return 8;
    case 0x67:
        Load8(H, A);
        return 4;
    // LD L, R2 -- Load value from R2 into L
    case 0x68:
        Load8(L, B);
        return 4;
    case 0x69:
        Load8(L, C);
        return 4;
    case 0x6A:
        Load8(L, D);
        return 4;
    case 0x6B:
        Load8(L, E);
        return 4;
    case 0x6C:
        Load8(L, H);
        return 4;
    case 0x6D:
        Load8(L, L);
        return 4;
    case 0x6E:
        Load8FromMem(L, regs.reg16[HL]);
        return 8;
    case 0x6F:
        Load8(L, A);
        return 4;
    // LD (HL), R2 -- Load value from R2 into memory at (HL)
    case 0x70:
        Load8IntoMem(regs.reg16[HL], B);
        return 8;
    case 0x71:
        Load8IntoMem(regs.reg16[HL], C);
        return 8;
    case 0x72:
        Load8IntoMem(regs.reg16[HL], D);
        return 8;
    case 0x73:
        Load8IntoMem(regs.reg16[HL], E);
        return 8;
    case 0x74:
        Load8IntoMem(regs.reg16[HL], H);
        return 8;
    case 0x75:
        Load8IntoMem(regs.reg16[HL], L);
        return 8;
    case 0x77:
        Load8IntoMem(regs.reg16[HL], A);
        return 8;
    case 0x36:
        Load8IntoMemImmediate(regs.reg16[HL], GetImmediateByte());
        return 12;
    // LD A, (nn) -- Load value from memory at (nn) into A
    case 0x0A:
        Load8FromMem(A, regs.reg16[BC]);
        return 8;
    case 0x1A:
        Load8FromMem(A, regs.reg16[DE]);
        return 8;
    case 0xFA:
        Load8FromMem(A, GetImmediateWord());
        return 16;
    // LD (nn), A -- Load value from A into memory at (nn)
    case 0x02:
        Load8IntoMem(regs.reg16[BC], A);
        return 8;
    case 0x12:
        Load8IntoMem(regs.reg16[DE], A);
        return 8;
    case 0xEA:
        Load8IntoMem(GetImmediateWord(), A);
        return 16;
    // LD (C), A -- Load value from A into memory at (0xFF00 + C)
    case 0xE2:
        Load8IntoMem(0xFF00 + regs.reg8[C], A);
        return 8;
    // LD A, (C) -- Load value from memory at (0xFF00 + C) into A
    case 0xF2:
        Load8FromMem(A, 0xFF00 + regs.reg8[C]);
        return 8;
    // LDI (HL), A -- Load value from A into memory at (HL), then increment HL
    case 0x22:
        Load8IntoMem(regs.reg16[HL]++, A);
        return 8;
    // LDI A, (HL) -- Load value from memory at (HL) into A, then increment HL
    case 0x2A:
        Load8FromMem(A, regs.reg16[HL]++);
        return 8;
    // LDD (HL), A -- Load value from A into memory at (HL), then decrement HL
    case 0x32:
        Load8IntoMem(regs.reg16[HL]--, A);
        return 8;
    // LDD A, (HL) -- Load value from memory at (HL) into A, then decrement HL
    case 0x3A:
        Load8FromMem(A, regs.reg16[HL]--);
        return 8;
    // LDH (n), A -- Load value from A into memory at (0xFF00+n), with n as immediate byte value
    case 0xE0:
        Load8IntoMem(0xFF00 + GetImmediateByte(), A);
        return 12;
    // LDH A, (n) -- Load value from memory at (0xFF00+n) into A, with n as immediate byte value 
    case 0xF0:
        Load8FromMem(A, 0xFF00 + GetImmediateByte());
        return 12;

    // ******** 16-bit loads ********
    // LD R, nn -- Load 16-bit immediate value into 16-bit register R
    case 0x01:
        Load16Immediate(BC, GetImmediateWord());
        return 12;
    case 0x11:
        Load16Immediate(DE, GetImmediateWord());
        return 12;
    case 0x21:
        Load16Immediate(HL, GetImmediateWord());
        return 12;
    case 0x31:
        Load16Immediate(SP, GetImmediateWord());
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
        LoadSPnIntoHL(GetImmediateByte());
        return 12;
    // LD (nn), SP -- Load value from SP into memory at (nn)
    case 0x08:
        LoadSPIntoMem(GetImmediateWord());
        return 20;
    // PUSH R -- Push 16-bit register R onto the stack and decrement the stack pointer by 2
    case 0xC5:
        Push(BC);
        return 16;
    case 0xD5:
        Push(DE);
        return 16;
    case 0xE5:
        Push(HL);
        return 16;
    case 0xF5:
        Push(AF);
        return 16;
    // POP R -- Pop 2 bytes off the stack into 16-bit register R and increment the stack pointer by 2
    case 0xC1:
        Pop(BC);
        return 12;
    case 0xD1:
        Pop(DE);
        return 12;
    case 0xE1:
        Pop(HL);
        return 12;
    case 0xF1:
        Pop(AF);
        return 12;

    // ******** 8-bit arithmetic and logic ********
    // ADD A, R -- Add value in register R to A
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Set if carry from bit 7
    case 0x80:
        Add(B);
        return 4;
    case 0x81:
        Add(C);
        return 4;
    case 0x82:
        Add(D);
        return 4;
    case 0x83:
        Add(E);
        return 4;
    case 0x84:
        Add(H);
        return 4;
    case 0x85:
        Add(L);
        return 4;
    case 0x86:
        AddFromMemAtHL();
        return 8;
    case 0x87:
        Add(A);
        return 4;
    // ADD A, n -- Add immediate value n to A
    // Flags: same as ADD A, R
    case 0xC6:
        AddImmediate(GetImmediateByte());
        return 8;
    // ADC A, R -- Add value in register R + the carry flag to A
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Set if carry from bit 7
    case 0x88:
        AddWithCarry(B);
        return 4;
    case 0x89:
        AddWithCarry(C);
        return 4;
    case 0x8A:
        AddWithCarry(D);
        return 4;
    case 0x8B:
        AddWithCarry(E);
        return 4;
    case 0x8C:
        AddWithCarry(H);
        return 4;
    case 0x8D:
        AddWithCarry(L);
        return 4;
    case 0x8E:
        AddFromMemAtHLWithCarry();
        return 8;
    case 0x8F:
        AddWithCarry(A);
        return 4;
    // ADC A, n -- Add immediate value n + the carry flag to A
    // Flags: same as ADC A, R
    case 0xCE:
        AddImmediateWithCarry(GetImmediateByte());
        return 8;
    // SUB R -- Subtract the value in register R from  A
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0x90:
        Sub(B);
        return 4;
    case 0x91:
        Sub(C);
        return 4;
    case 0x92:
        Sub(D);
        return 4;
    case 0x93:
        Sub(E);
        return 4;
    case 0x94:
        Sub(H);
        return 4;
    case 0x95:
        Sub(L);
        return 4;
    case 0x96:
        SubFromMemAtHL();
        return 8;
    case 0x97:
        Sub(A);
        return 4;
    // SUB n -- Subtract immediate value n from  A
    // Flags: same as SUB R
    case 0xD6:
        SubImmediate(GetImmediateByte());
        return 8;
    // SBC A, R -- Subtract the value in register R + carry flag from  A
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0x98:
        SubWithCarry(B);
        return 4;
    case 0x99:
        SubWithCarry(C);
        return 4;
    case 0x9A:
        SubWithCarry(D);
        return 4;
    case 0x9B:
        SubWithCarry(E);
        return 4;
    case 0x9C:
        SubWithCarry(H);
        return 4;
    case 0x9D:
        SubWithCarry(L);
        return 4;
    case 0x9E:
        SubFromMemAtHLWithCarry();
        return 8;
    case 0x9F:
        SubWithCarry(A);
        return 4;
    // SBC A, n -- Subtract immediate value n + carry flag from  A
    // Flags: same as SBC A, R
    case 0xDE:
        SubImmediateWithCarry(GetImmediateByte());
        return 8;
    // AND R -- Bitwise AND the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set
    //     C: Reset
    case 0xA0:
        And(B);
        return 4;
    case 0xA1:
        And(C);
        return 4;
    case 0xA2:
        And(D);
        return 4;
    case 0xA3:
        And(E);
        return 4;
    case 0xA4:
        And(H);
        return 4;
    case 0xA5:
        And(L);
        return 4;
    case 0xA6:
        AndFromMemAtHL();
        return 8;
    case 0xA7:
        And(A);
        return 4;
    // AND n -- Bitwise AND the immediate value with A. 
    // Flags: same as AND R
    case 0xE6:
        AndImmediate(GetImmediateByte());
        return 8;
    // OR R -- Bitwise OR the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Reset
    //     C: Reset
    case 0xB0:
        Or(B);
        return 4;
    case 0xB1:
        Or(C);
        return 4;
    case 0xB2:
        Or(D);
        return 4;
    case 0xB3:
        Or(E);
        return 4;
    case 0xB4:
        Or(H);
        return 4;
    case 0xB5:
        Or(L);
        return 4;
    case 0xB6:
        OrFromMemAtHL();
        return 8;
    case 0xB7:
        Or(A);
        return 4;
    // OR n -- Bitwise OR the immediate value with A. 
    // Flags: same as OR R
    case 0xF6:
        OrImmediate(GetImmediateByte());
        return 8;
    // XOR R -- Bitwise XOR the value in register R with A. 
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Reset
    //     C: Reset
    case 0xA8:
        Xor(B);
        return 4;
    case 0xA9:
        Xor(C);
        return 4;
    case 0xAA:
        Xor(D);
        return 4;
    case 0xAB:
        Xor(E);
        return 4;
    case 0xAC:
        Xor(H);
        return 4;
    case 0xAD:
        Xor(L);
        return 4;
    case 0xAE:
        XorFromMemAtHL();
        return 8;
    case 0xAF:
        Xor(A);
        return 4;
    // XOR n -- Bitwise XOR the immediate value with A. 
    // Flags: same as XOR R
    case 0xEE:
        XorImmediate(GetImmediateByte());
        return 8;
    // CP R -- Compare A with the value in register R. This performs a subtraction but does not modify A.
    // Flags:
    //     Z: Set if result is zero, i.e. A is equal to R
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Set if borrow
    case 0xB8:
        Compare(B);
        return 4;
    case 0xB9:
        Compare(C);
        return 4;
    case 0xBA:
        Compare(D);
        return 4;
    case 0xBB:
        Compare(E);
        return 4;
    case 0xBC:
        Compare(H);
        return 4;
    case 0xBD:
        Compare(L);
        return 4;
    case 0xBE:
        CompareFromMemAtHL();
        return 8;
    case 0xBF:
        Compare(A);
        return 4;
    // CP n -- Compare A with the immediate value. This performs a subtraction but does not modify A.
    // Flags: same as CP R
    case 0xFE:
        CompareImmediate(GetImmediateByte());
        return 8;
    // INC R -- Increment the value in register R.
    // Flags:
    //     Z: Set if result is zero
    //     N: Reset
    //     H: Set if carry from bit 3
    //     C: Unchanged
    case 0x04:
        IncReg8(B);
        return 4;
    case 0x0C:
        IncReg8(C);
        return 4;
    case 0x14:
        IncReg8(D);
        return 4;
    case 0x1C:
        IncReg8(E);
        return 4;
    case 0x24:
        IncReg8(H);
        return 4;
    case 0x2C:
        IncReg8(L);
        return 4;
    case 0x34:
        IncMemAtHL();
        return 12;
    case 0x3C:
        IncReg8(A);
        return 4;
    // DEC R -- Decrement the value in register R.
    // Flags:
    //     Z: Set if result is zero
    //     N: Set
    //     H: Set if borrow from bit 4
    //     C: Unchanged
    case 0x05:
        DecReg8(B);
        return 4;
    case 0x0D:
        DecReg8(C);
        return 4;
    case 0x15:
        DecReg8(D);
        return 4;
    case 0x1D:
        DecReg8(E);
        return 4;
    case 0x25:
        DecReg8(H);
        return 4;
    case 0x2D:
        DecReg8(L);
        return 4;
    case 0x35:
        DecMemAtHL();
        return 12;
    case 0x3D:
        DecReg8(A);
        return 4;

    // ******** 16-bit arithmetic ********
    // ADD HL, R -- Add the value in the 16-bit register R to HL.
    // Flags:
    //     Z: Unchanged
    //     N: Reset
    //     H: Set if carry from bit 11
    //     C: Set if carry from bit 15
    case 0x09:
        AddHL(BC);
        return 8;
    case 0x19:
        AddHL(DE);
        return 8;
    case 0x29:
        AddHL(HL);
        return 8;
    case 0x39:
        AddHL(SP);
        return 8;
    // ADD SP, n -- Add signed immediate byte to SP.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Set appropriately, with immediate as unsigned byte.
    //     C: Set appropriately, with immediate as unsigned byte.
    case 0xE8:
        AddSP(GetImmediateByte());
        return 16;
    // INC R -- Increment the value in the 16-bit register R.
    // Flags unchanged
    case 0x03:
        IncReg16(BC);
        return 8;
    case 0x13:
        IncReg16(DE);
        return 8;
    case 0x23:
        IncReg16(HL);
        return 8;
    case 0x33:
        IncReg16(SP);
        return 8;
    // DEC R -- Decrement the value in the 16-bit register R.
    // Flags unchanged
    case 0x0B:
        DecReg16(BC);
        return 8;
    case 0x1B:
        DecReg16(DE);
        return 8;
    case 0x2B:
        DecReg16(HL);
        return 8;
    case 0x3B:
        DecReg16(SP);
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
        RotateLeft(A);
        SetZero(false);
        return 4;
    // RLA -- Left rotate A through the carry flag.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 7 before the rotate
    case 0x17:
        RotateLeftThroughCarry(A);
        SetZero(false);
        return 4;
    // RRCA -- Right rotate A.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 0 before the rotate
    case 0x0F:
        RotateRight(A);
        SetZero(false);
        return 4;
    // RRA -- Right rotate A through the carry flag.
    // Flags:
    //     Z: Reset
    //     N: Reset
    //     H: Reset
    //     C: Set to value in bit 0 before the rotate
    case 0x1F:
        RotateRightThroughCarry(A);
        SetZero(false);
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
        if (!Zero()) {
            Jump(GetImmediateWord());
            return 16;
        } else {
            gameboy.HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xCA:
        if (Zero()) {
            Jump(GetImmediateWord());
            return 16;
        } else {
            gameboy.HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xD2:
        if (!Carry()) {
            Jump(GetImmediateWord());
            return 16;
        } else {
            gameboy.HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xDA:
        if (Carry()) {
            Jump(GetImmediateWord());
            return 16;
        } else {
            gameboy.HardwareTick(8);
            pc += 2;
            return 12;
        }
    // JP (HL) -- Jump to the address contained in HL.
    case 0xE9:
        JumpToHL();
        return 4;
    // JR n -- Jump to the current address + immediate signed byte.
    case 0x18:
        RelativeJump(GetImmediateByte());
        return 12;
    // JR cc, n -- Jump to the current address + immediate signed byte if the specified condition is true.
    // cc ==
    //     NZ: Zero flag reset
    //     Z:  Zero flag set
    //     NC: Carry flag reset
    //     Z:  Carry flag set
    case 0x20:
        if (!Zero()) {
            RelativeJump(GetImmediateByte());
            return 12;
        } else {
            gameboy.HardwareTick(4);
            ++pc;
            return 8;
        }
    case 0x28:
        if (Zero()) {
            RelativeJump(GetImmediateByte());
            return 12;
        } else {
            gameboy.HardwareTick(4);
            ++pc;
            return 8;
        }
    case 0x30:
        if (!Carry()) {
            RelativeJump(GetImmediateByte());
            return 12;
        } else {
            gameboy.HardwareTick(4);
            ++pc;
            return 8;
        }
    case 0x38:
        if (Carry()) {
            RelativeJump(GetImmediateByte());
            return 12;
        } else {
            gameboy.HardwareTick(4);
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
        if (!Zero()) {
            Call(GetImmediateWord());
            return 24;
        } else {
            gameboy.HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xCC:
        if (Zero()) {
            Call(GetImmediateWord());
            return 24;
        } else {
            gameboy.HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xD4:
        if (!Carry()) {
            Call(GetImmediateWord());
            return 24;
        } else {
            gameboy.HardwareTick(8);
            pc += 2;
            return 12;
        }
    case 0xDC:
        if (Carry()) {
            Call(GetImmediateWord());
            return 24;
        } else {
            gameboy.HardwareTick(8);
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
        gameboy.HardwareTick(4); // For the comparison.
        if (!Zero()) {
            Return();
            return 20;
        } else {
            return 8;
        }
    case 0xC8:
        gameboy.HardwareTick(4); // For the comparison.
        if (Zero()) {
            Return();
            return 20;
        } else {
            return 8;
        }
    case 0xD0:
        gameboy.HardwareTick(4); // For the comparison.
        if (!Carry()) {
            Return();
            return 20;
        } else {
            return 8;
        }
    case 0xD8:
        gameboy.HardwareTick(4); // For the comparison.
        if (Carry()) {
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
            RotateLeft(B);
            return 8;
        case 0x01:
            RotateLeft(C);
            return 8;
        case 0x02:
            RotateLeft(D);
            return 8;
        case 0x03:
            RotateLeft(E);
            return 8;
        case 0x04:
            RotateLeft(H);
            return 8;
        case 0x05:
            RotateLeft(L);
            return 8;
        case 0x06:
            RotateLeftMemAtHL();
            return 16;
        case 0x07:
            RotateLeft(A);
            return 8;
        // RL R -- Left rotate the value in register R through the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 7 before the rotate
        case 0x10:
            RotateLeftThroughCarry(B);
            return 8;
        case 0x11:
            RotateLeftThroughCarry(C);
            return 8;
        case 0x12:
            RotateLeftThroughCarry(D);
            return 8;
        case 0x13:
            RotateLeftThroughCarry(E);
            return 8;
        case 0x14:
            RotateLeftThroughCarry(H);
            return 8;
        case 0x15:
            RotateLeftThroughCarry(L);
            return 8;
        case 0x16:
            RotateLeftMemAtHLThroughCarry();
            return 16;
        case 0x17:
            RotateLeftThroughCarry(A);
            return 8;
        // RRC R -- Right rotate the value in register R.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x08:
            RotateRight(B);
            return 8;
        case 0x09:
            RotateRight(C);
            return 8;
        case 0x0A:
            RotateRight(D);
            return 8;
        case 0x0B:
            RotateRight(E);
            return 8;
        case 0x0C:
            RotateRight(H);
            return 8;
        case 0x0D:
            RotateRight(L);
            return 8;
        case 0x0E:
            RotateRightMemAtHL();
            return 16;
        case 0x0F:
            RotateRight(A);
            return 8;
        // RR R -- Right rotate the value in register R through the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x18:
            RotateRightThroughCarry(B);
            return 8;
        case 0x19:
            RotateRightThroughCarry(C);
            return 8;
        case 0x1A:
            RotateRightThroughCarry(D);
            return 8;
        case 0x1B:
            RotateRightThroughCarry(E);
            return 8;
        case 0x1C:
            RotateRightThroughCarry(H);
            return 8;
        case 0x1D:
            RotateRightThroughCarry(L);
            return 8;
        case 0x1E:
            RotateRightMemAtHLThroughCarry();
            return 16;
        case 0x1F:
            RotateRightThroughCarry(A);
            return 8;
        // SLA R -- Left shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x20:
            ShiftLeft(B);
            return 8;
        case 0x21:
            ShiftLeft(C);
            return 8;
        case 0x22:
            ShiftLeft(D);
            return 8;
        case 0x23:
            ShiftLeft(E);
            return 8;
        case 0x24:
            ShiftLeft(H);
            return 8;
        case 0x25:
            ShiftLeft(L);
            return 8;
        case 0x26:
            ShiftLeftMemAtHL();
            return 16;
        case 0x27:
            ShiftLeft(A);
            return 8;
        // SRA R -- Arithmetic right shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x28:
            ShiftRightArithmetic(B);
            return 8;
        case 0x29:
            ShiftRightArithmetic(C);
            return 8;
        case 0x2A:
            ShiftRightArithmetic(D);
            return 8;
        case 0x2B:
            ShiftRightArithmetic(E);
            return 8;
        case 0x2C:
            ShiftRightArithmetic(H);
            return 8;
        case 0x2D:
            ShiftRightArithmetic(L);
            return 8;
        case 0x2E:
            ShiftRightArithmeticMemAtHL();
            return 16;
        case 0x2F:
            ShiftRightArithmetic(A);
            return 8;
        // SWAP R -- Swap upper and lower nybbles of register R (rotate by 4).
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Reset
        case 0x30:
            SwapNybbles(B);
            return 8;
        case 0x31:
            SwapNybbles(C);
            return 8;
        case 0x32:
            SwapNybbles(D);
            return 8;
        case 0x33:
            SwapNybbles(E);
            return 8;
        case 0x34:
            SwapNybbles(H);
            return 8;
        case 0x35:
            SwapNybbles(L);
            return 8;
        case 0x36:
            SwapMemAtHL();
            return 16;
        case 0x37:
            SwapNybbles(A);
            return 8;
        // SRL R -- Logical right shift the value in register R into the carry flag.
        // Flags:
        //     Z: Set if result is zero
        //     N: Reset
        //     H: Reset
        //     C: Set to value in bit 0 before the rotate
        case 0x38:
            ShiftRightLogical(B);
            return 8;
        case 0x39:
            ShiftRightLogical(C);
            return 8;
        case 0x3A:
            ShiftRightLogical(D);
            return 8;
        case 0x3B:
            ShiftRightLogical(E);
            return 8;
        case 0x3C:
            ShiftRightLogical(H);
            return 8;
        case 0x3D:
            ShiftRightLogical(L);
            return 8;
        case 0x3E:
            ShiftRightLogicalMemAtHL();
            return 16;
        case 0x3F:
            ShiftRightLogical(A);
            return 8;

        // ******** Bit Manipulation ********
        // BIT b, R -- test bit b of the value in register R.
        // Flags:
        //     Z: Set if bit b of R is zero
        //     N: Reset
        //     H: Set
        //     C: Unchanged
        case 0x40:
            TestBit(0, B);
            return 8;
        case 0x41:
            TestBit(0, C);
            return 8;
        case 0x42:
            TestBit(0, D);
            return 8;
        case 0x43:
            TestBit(0, E);
            return 8;
        case 0x44:
            TestBit(0, H);
            return 8;
        case 0x45:
            TestBit(0, L);
            return 8;
        case 0x46:
            TestBitOfMemAtHL(0);
            return 12;
        case 0x47:
            TestBit(0, A);
            return 8;
        case 0x48:
            TestBit(1, B);
            return 8;
        case 0x49:
            TestBit(1, C);
            return 8;
        case 0x4A:
            TestBit(1, D);
            return 8;
        case 0x4B:
            TestBit(1, E);
            return 8;
        case 0x4C:
            TestBit(1, H);
            return 8;
        case 0x4D:
            TestBit(1, L);
            return 8;
        case 0x4E:
            TestBitOfMemAtHL(1);
            return 12;
        case 0x4F:
            TestBit(1, A);
            return 8;
        case 0x50:
            TestBit(2, B);
            return 8;
        case 0x51:
            TestBit(2, C);
            return 8;
        case 0x52:
            TestBit(2, D);
            return 8;
        case 0x53:
            TestBit(2, E);
            return 8;
        case 0x54:
            TestBit(2, H);
            return 8;
        case 0x55:
            TestBit(2, L);
            return 8;
        case 0x56:
            TestBitOfMemAtHL(2);
            return 12;
        case 0x57:
            TestBit(2, A);
            return 8;
        case 0x58:
            TestBit(3, B);
            return 8;
        case 0x59:
            TestBit(3, C);
            return 8;
        case 0x5A:
            TestBit(3, D);
            return 8;
        case 0x5B:
            TestBit(3, E);
            return 8;
        case 0x5C:
            TestBit(3, H);
            return 8;
        case 0x5D:
            TestBit(3, L);
            return 8;
        case 0x5E:
            TestBitOfMemAtHL(3);
            return 12;
        case 0x5F:
            TestBit(3, A);
            return 8;
        case 0x60:
            TestBit(4, B);
            return 8;
        case 0x61:
            TestBit(4, C);
            return 8;
        case 0x62:
            TestBit(4, D);
            return 8;
        case 0x63:
            TestBit(4, E);
            return 8;
        case 0x64:
            TestBit(4, H);
            return 8;
        case 0x65:
            TestBit(4, L);
            return 8;
        case 0x66:
            TestBitOfMemAtHL(4);
            return 12;
        case 0x67:
            TestBit(4, A);
            return 8;
        case 0x68:
            TestBit(5, B);
            return 8;
        case 0x69:
            TestBit(5, C);
            return 8;
        case 0x6A:
            TestBit(5, D);
            return 8;
        case 0x6B:
            TestBit(5, E);
            return 8;
        case 0x6C:
            TestBit(5, H);
            return 8;
        case 0x6D:
            TestBit(5, L);
            return 8;
        case 0x6E:
            TestBitOfMemAtHL(5);
            return 12;
        case 0x6F:
            TestBit(5, A);
            return 8;
        case 0x70:
            TestBit(6, B);
            return 8;
        case 0x71:
            TestBit(6, C);
            return 8;
        case 0x72:
            TestBit(6, D);
            return 8;
        case 0x73:
            TestBit(6, E);
            return 8;
        case 0x74:
            TestBit(6, H);
            return 8;
        case 0x75:
            TestBit(6, L);
            return 8;
        case 0x76:
            TestBitOfMemAtHL(6);
            return 12;
        case 0x77:
            TestBit(6, A);
            return 8;
        case 0x78:
            TestBit(7, B);
            return 8;
        case 0x79:
            TestBit(7, C);
            return 8;
        case 0x7A:
            TestBit(7, D);
            return 8;
        case 0x7B:
            TestBit(7, E);
            return 8;
        case 0x7C:
            TestBit(7, H);
            return 8;
        case 0x7D:
            TestBit(7, L);
            return 8;
        case 0x7E:
            TestBitOfMemAtHL(7);
            return 12;
        case 0x7F:
            TestBit(7, A);
            return 8;
        // RES b, R -- reset bit b of the value in register R.
        // Flags unchanged
        case 0x80:
            ResetBit(0, B);
            return 8;
        case 0x81:
            ResetBit(0, C);
            return 8;
        case 0x82:
            ResetBit(0, D);
            return 8;
        case 0x83:
            ResetBit(0, E);
            return 8;
        case 0x84:
            ResetBit(0, H);
            return 8;
        case 0x85:
            ResetBit(0, L);
            return 8;
        case 0x86:
            ResetBitOfMemAtHL(0);
            return 16;
        case 0x87:
            ResetBit(0, A);
            return 8;
        case 0x88:
            ResetBit(1, B);
            return 8;
        case 0x89:
            ResetBit(1, C);
            return 8;
        case 0x8A:
            ResetBit(1, D);
            return 8;
        case 0x8B:
            ResetBit(1, E);
            return 8;
        case 0x8C:
            ResetBit(1, H);
            return 8;
        case 0x8D:
            ResetBit(1, L);
            return 8;
        case 0x8E:
            ResetBitOfMemAtHL(1);
            return 16;
        case 0x8F:
            ResetBit(1, A);
            return 8;
        case 0x90:
            ResetBit(2, B);
            return 8;
        case 0x91:
            ResetBit(2, C);
            return 8;
        case 0x92:
            ResetBit(2, D);
            return 8;
        case 0x93:
            ResetBit(2, E);
            return 8;
        case 0x94:
            ResetBit(2, H);
            return 8;
        case 0x95:
            ResetBit(2, L);
            return 8;
        case 0x96:
            ResetBitOfMemAtHL(2);
            return 16;
        case 0x97:
            ResetBit(2, A);
            return 8;
        case 0x98:
            ResetBit(3, B);
            return 8;
        case 0x99:
            ResetBit(3, C);
            return 8;
        case 0x9A:
            ResetBit(3, D);
            return 8;
        case 0x9B:
            ResetBit(3, E);
            return 8;
        case 0x9C:
            ResetBit(3, H);
            return 8;
        case 0x9D:
            ResetBit(3, L);
            return 8;
        case 0x9E:
            ResetBitOfMemAtHL(3);
            return 16;
        case 0x9F:
            ResetBit(3, A);
            return 8;
        case 0xA0:
            ResetBit(4, B);
            return 8;
        case 0xA1:
            ResetBit(4, C);
            return 8;
        case 0xA2:
            ResetBit(4, D);
            return 8;
        case 0xA3:
            ResetBit(4, E);
            return 8;
        case 0xA4:
            ResetBit(4, H);
            return 8;
        case 0xA5:
            ResetBit(4, L);
            return 8;
        case 0xA6:
            ResetBitOfMemAtHL(4);
            return 16;
        case 0xA7:
            ResetBit(4, A);
            return 8;
        case 0xA8:
            ResetBit(5, B);
            return 8;
        case 0xA9:
            ResetBit(5, C);
            return 8;
        case 0xAA:
            ResetBit(5, D);
            return 8;
        case 0xAB:
            ResetBit(5, E);
            return 8;
        case 0xAC:
            ResetBit(5, H);
            return 8;
        case 0xAD:
            ResetBit(5, L);
            return 8;
        case 0xAE:
            ResetBitOfMemAtHL(5);
            return 16;
        case 0xAF:
            ResetBit(5, A);
            return 8;
        case 0xB0:
            ResetBit(6, B);
            return 8;
        case 0xB1:
            ResetBit(6, C);
            return 8;
        case 0xB2:
            ResetBit(6, D);
            return 8;
        case 0xB3:
            ResetBit(6, E);
            return 8;
        case 0xB4:
            ResetBit(6, H);
            return 8;
        case 0xB5:
            ResetBit(6, L);
            return 8;
        case 0xB6:
            ResetBitOfMemAtHL(6);
            return 16;
        case 0xB7:
            ResetBit(6, A);
            return 8;
        case 0xB8:
            ResetBit(7, B);
            return 8;
        case 0xB9:
            ResetBit(7, C);
            return 8;
        case 0xBA:
            ResetBit(7, D);
            return 8;
        case 0xBB:
            ResetBit(7, E);
            return 8;
        case 0xBC:
            ResetBit(7, H);
            return 8;
        case 0xBD:
            ResetBit(7, L);
            return 8;
        case 0xBE:
            ResetBitOfMemAtHL(7);
            return 16;
        case 0xBF:
            ResetBit(7, A);
            return 8;
        // SET b, R -- set bit b of the value in register R.
        // Flags unchanged
        case 0xC0:
            SetBit(0, B);
            return 8;
        case 0xC1:
            SetBit(0, C);
            return 8;
        case 0xC2:
            SetBit(0, D);
            return 8;
        case 0xC3:
            SetBit(0, E);
            return 8;
        case 0xC4:
            SetBit(0, H);
            return 8;
        case 0xC5:
            SetBit(0, L);
            return 8;
        case 0xC6:
            SetBitOfMemAtHL(0);
            return 16;
        case 0xC7:
            SetBit(0, A);
            return 8;
        case 0xC8:
            SetBit(1, B);
            return 8;
        case 0xC9:
            SetBit(1, C);
            return 8;
        case 0xCA:
            SetBit(1, D);
            return 8;
        case 0xCB:
            SetBit(1, E);
            return 8;
        case 0xCC:
            SetBit(1, H);
            return 8;
        case 0xCD:
            SetBit(1, L);
            return 8;
        case 0xCE:
            SetBitOfMemAtHL(1);
            return 16;
        case 0xCF:
            SetBit(1, A);
            return 8;
        case 0xD0:
            SetBit(2, B);
            return 8;
        case 0xD1:
            SetBit(2, C);
            return 8;
        case 0xD2:
            SetBit(2, D);
            return 8;
        case 0xD3:
            SetBit(2, E);
            return 8;
        case 0xD4:
            SetBit(2, H);
            return 8;
        case 0xD5:
            SetBit(2, L);
            return 8;
        case 0xD6:
            SetBitOfMemAtHL(2);
            return 16;
        case 0xD7:
            SetBit(2, A);
            return 8;
        case 0xD8:
            SetBit(3, B);
            return 8;
        case 0xD9:
            SetBit(3, C);
            return 8;
        case 0xDA:
            SetBit(3, D);
            return 8;
        case 0xDB:
            SetBit(3, E);
            return 8;
        case 0xDC:
            SetBit(3, H);
            return 8;
        case 0xDD:
            SetBit(3, L);
            return 8;
        case 0xDE:
            SetBitOfMemAtHL(3);
            return 16;
        case 0xDF:
            SetBit(3, A);
            return 8;
        case 0xE0:
            SetBit(4, B);
            return 8;
        case 0xE1:
            SetBit(4, C);
            return 8;
        case 0xE2:
            SetBit(4, D);
            return 8;
        case 0xE3:
            SetBit(4, E);
            return 8;
        case 0xE4:
            SetBit(4, H);
            return 8;
        case 0xE5:
            SetBit(4, L);
            return 8;
        case 0xE6:
            SetBitOfMemAtHL(4);
            return 16;
        case 0xE7:
            SetBit(4, A);
            return 8;
        case 0xE8:
            SetBit(5, B);
            return 8;
        case 0xE9:
            SetBit(5, C);
            return 8;
        case 0xEA:
            SetBit(5, D);
            return 8;
        case 0xEB:
            SetBit(5, E);
            return 8;
        case 0xEC:
            SetBit(5, H);
            return 8;
        case 0xED:
            SetBit(5, L);
            return 8;
        case 0xEE:
            SetBitOfMemAtHL(5);
            return 16;
        case 0xEF:
            SetBit(5, A);
            return 8;
        case 0xF0:
            SetBit(6, B);
            return 8;
        case 0xF1:
            SetBit(6, C);
            return 8;
        case 0xF2:
            SetBit(6, D);
            return 8;
        case 0xF3:
            SetBit(6, E);
            return 8;
        case 0xF4:
            SetBit(6, H);
            return 8;
        case 0xF5:
            SetBit(6, L);
            return 8;
        case 0xF6:
            SetBitOfMemAtHL(6);
            return 16;
        case 0xF7:
            SetBit(6, A);
            return 8;
        case 0xF8:
            SetBit(7, B);
            return 8;
        case 0xF9:
            SetBit(7, C);
            return 8;
        case 0xFA:
            SetBit(7, D);
            return 8;
        case 0xFB:
            SetBit(7, E);
            return 8;
        case 0xFC:
            SetBit(7, H);
            return 8;
        case 0xFD:
            SetBit(7, L);
            return 8;
        case 0xFE:
            SetBitOfMemAtHL(7);
            return 16;
        case 0xFF:
            SetBit(7, A);
            return 8;

        default:
            // Unreachable, every possible case is specified above.
            return 8;
        }

    default:
        throw std::runtime_error("The CPU has hung. Reason: unknown opcode.");
        return 4;
    }
}

} // End namespace Gb
