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

#include <cassert>

#include "core/cpu/CPU.h"
#include "core/memory/Memory.h"
#include "core/GameBoy.h"

namespace Core {

// 8-bit Load operations
void CPU::Load8(Reg8 R, u8 immediate) {
    Write8(R, immediate);
}

void CPU::Load8FromMem(Reg8 R, u16 addr) {
    Write8(R, mem.ReadMem8(addr));
    gameboy->HardwareTick(4);
}

void CPU::Load8FromMemAtHL(Reg8 R) {
    Write8(R, ReadMemAtHL());
}

void CPU::Load8IntoMem(Reg16 R, u8 immediate) {
    mem.WriteMem8(Read16(R), immediate);
    gameboy->HardwareTick(4);
}

void CPU::LoadAIntoMem(u16 addr) {
    mem.WriteMem8(addr, a);
    gameboy->HardwareTick(4);
}

// 16-bit Load operations
void CPU::Load16(Reg16 R, u16 immediate) {
    Write16(R, immediate);
}

void CPU::LoadHLIntoSP() {
    sp = (static_cast<u16>(h) << 8) | static_cast<u16>(l);
    gameboy->HardwareTick(4);
}

void CPU::LoadSPnIntoHL(s8 immediate) {
    // The half carry & carry flags for this instruction are set by adding the immediate as an *unsigned* byte to the
    // lower byte of sp. The addition itself is done with the immediate as a signed byte.
    f.SetZero(false);
    f.SetSub(false);

    f.SetHalf(((sp & 0x000F) + (static_cast<u8>(immediate) & 0x0F)) & 0x0010);
    f.SetCarry(((sp & 0x00FF) + static_cast<u8>(immediate)) & 0x0100);

    u16 tmp16 = sp + immediate;
    l = static_cast<u8>(tmp16);
    h = static_cast<u8>(tmp16 >> 8);
    
    // Internal delay
    gameboy->HardwareTick(4);
}

void CPU::LoadSPIntoMem(u16 addr) {
    mem.WriteMem8(addr, static_cast<u8>(sp));
    gameboy->HardwareTick(4);

    mem.WriteMem8(addr+1, static_cast<u8>(sp >> 8));
    gameboy->HardwareTick(4);
}

void CPU::Push(Reg16 R) {
    // Internal delay
    gameboy->HardwareTick(4);

    u16 reg_val = Read16(R); // TODO: clean up this register access mess

    mem.WriteMem8(--sp, static_cast<u8>(reg_val >> 8));
    gameboy->HardwareTick(4);

    mem.WriteMem8(--sp, static_cast<u8>(reg_val));
    gameboy->HardwareTick(4);
}

void CPU::Pop(Reg16 R) {
    u8 byte_lo = mem.ReadMem8(sp++);
    gameboy->HardwareTick(4);
    u8 byte_hi = mem.ReadMem8(sp++);
    gameboy->HardwareTick(4);

    Write16(R, (static_cast<u16>(byte_hi) << 8) | static_cast<u16>(byte_lo)); // TODO: clean up
}

// 8-bit Add operations
void CPU::Add(u8 immediate) {
    u16 tmp16 = a + immediate;
    f.SetHalf(((a & 0x0F) + (immediate & 0x0F)) > 0x0F);
    f.SetCarry(tmp16 > 0x00FF);
    f.SetZero(!(tmp16 & 0x00FF));
    f.SetSub(false);

    a = static_cast<u8>(tmp16);
}

void CPU::AddFromMemAtHL() {
    u8 val = ReadMemAtHL();
    u16 tmp16 = a + val;
    f.SetHalf(((a & 0x0F) + (val & 0x0F)) & 0x10);
    f.SetCarry(tmp16 & 0x0100);
    f.SetZero(!(tmp16 & 0x00FF));
    f.SetSub(false);

    a = static_cast<u8>(tmp16);
}

void CPU::AddWithCarry(u8 immediate) {
    u16 tmp16 = a + immediate + f.Carry();
    f.SetHalf(((a & 0x0F) + (immediate & 0x0F) + f.Carry()) & 0x10);
    f.SetCarry(tmp16 & 0x0100);
    f.SetZero(!(tmp16 & 0x00FF));
    f.SetSub(false);

    a = static_cast<u8>(tmp16);
}

void CPU::AddFromMemAtHLWithCarry() {
    u8 val = ReadMemAtHL();
    u16 tmp16 = a + val + f.Carry();
    f.SetHalf(((a & 0x0F) + (val & 0x0F) + f.Carry()) & 0x10);
    f.SetCarry(tmp16 & 0x0100);
    f.SetZero(!(tmp16 & 0x00FF));
    f.SetSub(false);

    a = static_cast<u8>(tmp16);
}

// 8-bit Subtract operations
void CPU::Sub(u8 immediate) {
    f.SetHalf((a & 0x0F) < (immediate & 0x0F));
    f.SetCarry(a < immediate);
    f.SetSub(true);

    a -= immediate;
    f.SetZero(!a);
}

void CPU::SubFromMemAtHL() {
    u8 val = ReadMemAtHL();
    f.SetHalf((a & 0x0F) < (val & 0x0F));
    f.SetCarry(a < val);
    f.SetSub(true);

    a -= val;
    f.SetZero(!a);
}

void CPU::SubWithCarry(u8 immediate) {
    u8 carry = f.Carry();
    f.SetHalf((a & 0x0F) < (immediate & 0x0F) + carry);
    f.SetCarry(a < immediate + carry);
    f.SetSub(true);

    a -= immediate + carry;
    f.SetZero(!a);
}

void CPU::SubFromMemAtHLWithCarry() {
    u8 val = ReadMemAtHL();
    u8 carry = f.Carry();
    f.SetHalf((a & 0x0F) < (val & 0x0F) + carry);
    f.SetCarry(a < val + carry);
    f.SetSub(true);

    a -= val + carry;
    f.SetZero(!a);
}

// Bitwise And operations
void CPU::And(u8 immediate) {
    a &= immediate;

    f.SetZero(!a);
    f.SetSub(false);
    f.SetHalf(true);
    f.SetCarry(false);
}

void CPU::AndFromMemAtHL() {
    a &= ReadMemAtHL();

    f.SetZero(!a);
    f.SetSub(false);
    f.SetHalf(true);
    f.SetCarry(false);
}

// Bitwise Or operations
void CPU::Or(u8 immediate) {
    a |= immediate;

    f.SetZero(!a);
    f.SetSub(false);
    f.SetHalf(false);
    f.SetCarry(false);
}

void CPU::OrFromMemAtHL() {
    a |= ReadMemAtHL();

    f.SetZero(!a);
    f.SetSub(false);
    f.SetHalf(false);
    f.SetCarry(false);
}

// Bitwise Xor operations
void CPU::Xor(u8 immediate) {
    a ^= immediate;

    f.SetZero(!a);
    f.SetSub(false);
    f.SetHalf(false);
    f.SetCarry(false);
}

void CPU::XorFromMemAtHL() {
    a ^= ReadMemAtHL();

    f.SetZero(!a);
    f.SetSub(false);
    f.SetHalf(false);
    f.SetCarry(false);
}

// Compare operations
void CPU::Compare(u8 immediate) {
    f.SetZero(!(a - immediate));
    f.SetSub(true);
    f.SetHalf((a & 0x0F) < (immediate & 0x0F));
    f.SetCarry(a < immediate);
}

void CPU::CompareFromMemAtHL() {
    u8 val = ReadMemAtHL();
    f.SetZero(!(a - val));
    f.SetSub(true);
    f.SetHalf((a & 0x0F) < (val & 0x0F));
    f.SetCarry(a < val);
}

// Increment operations
void CPU::IncReg(Reg8 R) {
    switch (R) {
    case (Reg8::A):
        f.SetHalf((a & 0x0F) == 0x0F);
        ++a;
        f.SetZero(!a);
        break;
    case (Reg8::B):
        f.SetHalf((b & 0x0F) == 0x0F);
        ++b;
        f.SetZero(!b);
        break;
    case (Reg8::C):
        f.SetHalf((c & 0x0F) == 0x0F);
        ++c;
        f.SetZero(!c);
        break;
    case (Reg8::D):
        f.SetHalf((d & 0x0F) == 0x0F);
        ++d;
        f.SetZero(!d);
        break;
    case (Reg8::E):
        f.SetHalf((e & 0x0F) == 0x0F);
        ++e;
        f.SetZero(!e);
        break;
    case (Reg8::H):
        f.SetHalf((h & 0x0F) == 0x0F);
        ++h;
        f.SetZero(!h);
        break;
    case (Reg8::L):
        f.SetHalf((l & 0x0F) == 0x0F);
        ++l;
        f.SetZero(!l);
        break;
    default:
        // Unreachable
        break;
    }

    f.SetSub(false);
}

void CPU::IncHL() {
    h += static_cast<u8>(((static_cast<u16>(l) + 1) & 0x0100) >> 8);
    ++l;
}

void CPU::IncReg(Reg16 R) {
    switch (R) {
    case (Reg16::BC):
        b += static_cast<u8>(((static_cast<u16>(c) + 1) & 0x0100) >> 8);
        ++c;
        break;
    case (Reg16::DE):
        d += static_cast<u8>(((static_cast<u16>(e) + 1) & 0x0100) >> 8);
        ++e;
        break;
    case (Reg16::HL):
        IncHL();
        break;
    case (Reg16::SP):
        ++sp;
        break;
    default:
        assert(false && "Reg16::AF passed to CPU::IncReg.");
        break;
    }

    gameboy->HardwareTick(4);
}

void CPU::IncMemAtHL() {
    u8 val = ReadMemAtHL();

    f.SetHalf((val & 0x0F) == 0x0F);
    ++val;
    f.SetZero(!val);
    f.SetSub(false);

    WriteMemAtHL(val);
}

// Decrement operations
void CPU::DecReg(Reg8 R) {
    switch (R) {
    case (Reg8::A):
        f.SetHalf((a & 0x0F) == 0x00);
        --a;
        f.SetZero(!a);
        break;
    case (Reg8::B):
        f.SetHalf((b & 0x0F) == 0x00);
        --b;
        f.SetZero(!b);
        break;
    case (Reg8::C):
        f.SetHalf((c & 0x0F) == 0x00);
        --c;
        f.SetZero(!c);
        break;
    case (Reg8::D):
        f.SetHalf((d & 0x0F) == 0x00);
        --d;
        f.SetZero(!d);
        break;
    case (Reg8::E):
        f.SetHalf((e & 0x0F) == 0x00);
        --e;
        f.SetZero(!e);
        break;
    case (Reg8::H):
        f.SetHalf((h & 0x0F) == 0x00);
        --h;
        f.SetZero(!h);
        break;
    case (Reg8::L):
        f.SetHalf((l & 0x0F) == 0x00);
        --l;
        f.SetZero(!l);
        break;
    default:
        // Unreachable
        break;
    }

    f.SetSub(true);
}

void CPU::DecHL() {
    u16 tmp16 = (static_cast<u16>(h) << 8) | static_cast<u16>(l);
    --tmp16;
    h = static_cast<u8>(tmp16 >> 8);
    l = static_cast<u8>(tmp16);
}

void CPU::DecReg(Reg16 R) {
    u16 tmp16;
    switch (R) {
    case (Reg16::BC):
        tmp16 = (static_cast<u16>(b) << 8) | static_cast<u16>(c);
        --tmp16;
        b = static_cast<u8>(tmp16 >> 8);
        c = static_cast<u8>(tmp16);
        break;
    case (Reg16::DE):
        tmp16 = (static_cast<u16>(d) << 8) | static_cast<u16>(e);
        --tmp16;
        d = static_cast<u8>(tmp16 >> 8);
        e = static_cast<u8>(tmp16);
        break;
    case (Reg16::HL):
        DecHL();
        break;
    case (Reg16::SP):
        --sp;
        break;
    default:
        assert(false && "Reg16::AF passed to CPU::DecReg.");
        break;
    }

    gameboy->HardwareTick(4);
}

void CPU::DecMemAtHL() {
    u8 val = ReadMemAtHL();

    f.SetHalf((val & 0x0F) == 0x00);
    --val;
    f.SetZero(!val);
    f.SetSub(true);

    WriteMemAtHL(val);
}

// 16-bit add operations
void CPU::AddHL(Reg16 R) {
    u16 lo;
    u16 hi;
    switch (R) {
    case (Reg16::BC):
        lo = c;
        hi = b;
        break;
    case (Reg16::DE):
        lo = e;
        hi = d;
        break;
    case (Reg16::HL):
        lo = l;
        hi = h;
        break;
    case (Reg16::SP):
        lo = sp & 0x00FF;
        hi = (sp & 0xFF00) >> 8;
        break;
    default:
        assert(false && "Reg16::AF passed to CPU::Add16.");
        break;
    }

    u16 tmp16 = l + lo;
    u16 lower_carry = (tmp16 & 0x0100) >> 8;
    l = static_cast<u8>(tmp16);

    tmp16 = h + hi + lower_carry;
    f.SetSub(false);
    f.SetHalf(((h & 0x0F) + (static_cast<u8>(hi) & 0x0F) + static_cast<u8>(lower_carry)) & 0x10);
    f.SetCarry(tmp16 & 0x0100);
    h = static_cast<u8>(tmp16);

    gameboy->HardwareTick(4);
}

void CPU::AddSP(s8 immediate) {
    // The half carry & carry flags for this instruction are set by adding the immediate as an *unsigned* byte to the
    // lower byte of sp. The addition itself is done with the immediate as a signed byte.
    f.SetZero(false);
    f.SetSub(false);

    f.SetHalf(((sp & 0x000F) + (static_cast<u8>(immediate) & 0x0F)) & 0x0010);
    f.SetCarry(((sp & 0x00FF) + static_cast<u8>(immediate)) & 0x0100);

    sp += immediate;

    // Two internal delays.
    gameboy->HardwareTick(8);
}

// Miscellaneous arithmetic
void CPU::DecimalAdjustA() {
    if (f.Sub()) {
        if (f.Carry()) {
            a -= 0x60;
        }
        if (f.Half()) {
            a -= 0x06;
        }
    } else {
        if (f.Carry() || a > 0x99) {
            a += 0x60;
            f.SetCarry(true);
        }
        if (f.Half() || (a & 0x0F) > 0x09) {
            a += 0x06;
        }
    }
    f.SetZero(!a);
    f.SetHalf(false);
}

void CPU::ComplementA() {
    a = ~a;
    f.SetSub(true);
    f.SetHalf(true);
}

void CPU::SetCarry() {
    f.SetCarry(true);
    f.SetSub(false);
    f.SetHalf(false);
}

void CPU::ComplementCarry() {
    f.SetCarry(!f.Carry());
    f.SetSub(false);
    f.SetHalf(false);
}

// Rotates and Shifts
void CPU::RotateLeft(Reg8 R) {
    switch (R) {
    case (Reg8::A):
        f.SetCarry(a & 0x80);

        a = (a << 1) | (a >> 7);

        f.SetZero(!a);
        break;
    case (Reg8::B):
        f.SetCarry(b & 0x80);

        b = (b << 1) | (b >> 7);

        f.SetZero(!b);
        break;
    case (Reg8::C):
        f.SetCarry(c & 0x80);

        c = (c << 1) | (c >> 7);

        f.SetZero(!c);
        break;
    case (Reg8::D):
        f.SetCarry(d & 0x80);

        d = (d << 1) | (d >> 7);

        f.SetZero(!d);
        break;
    case (Reg8::E):
        f.SetCarry(e & 0x80);

        e = (e << 1) | (e >> 7);

        f.SetZero(!e);
        break;
    case (Reg8::H):
        f.SetCarry(h & 0x80);

        h = (h << 1) | (h >> 7);

        f.SetZero(!h);
        break;
    case (Reg8::L):
        f.SetCarry(l & 0x80);

        l = (l << 1) | (l >> 7);

        f.SetZero(!l);
        break;
    default:
        // Unreachable
        break;
    }
    f.SetSub(false);
    f.SetHalf(false);
}

void CPU::RotateLeftMemAtHL() {
    u8 val = ReadMemAtHL();
    f.SetCarry(val & 0x80);

    val = (val << 1) | (val >> 7);

    f.SetZero(!val);
    f.SetSub(false);
    f.SetHalf(false);

    WriteMemAtHL(val);
}

void CPU::RotateLeftThroughCarry(Reg8 R) {
    u8 carry_val = 0x00;
    switch (R) {
    case (Reg8::A):
        carry_val = a & 0x80;

        a = (a << 1) | f.Carry();

        f.SetZero(!a);
        break;
    case (Reg8::B):
        carry_val = b & 0x80;

        b = (b << 1) | f.Carry();

        f.SetZero(!b);
        break;
    case (Reg8::C):
        carry_val = c & 0x80;

        c = (c << 1) | f.Carry();

        f.SetZero(!c);
        break;
    case (Reg8::D):
        carry_val = d & 0x80;

        d = (d << 1) | f.Carry();

        f.SetZero(!d);
        break;
    case (Reg8::E):
        carry_val = e & 0x80;

        e = (e << 1) | f.Carry();

        f.SetZero(!e);
        break;
    case (Reg8::H):
        carry_val = h & 0x80;

        h = (h << 1) | f.Carry();

        f.SetZero(!h);
        break;
    case (Reg8::L):
        carry_val = l & 0x80;

        l = (l << 1) | f.Carry();

        f.SetZero(!l);
        break;
    default:
        // Unreachable
        break;
    }
    f.SetSub(false);
    f.SetHalf(false);
    f.SetCarry(carry_val);
}

void CPU::RotateLeftMemAtHLThroughCarry() {
    u8 val = ReadMemAtHL();
    u8 carry_val = val & 0x80;

    val = (val << 1) | f.Carry();

    f.SetZero(!val);
    f.SetSub(false);
    f.SetHalf(false);
    f.SetCarry(carry_val);

    WriteMemAtHL(val);
}

void CPU::RotateRight(Reg8 R) {
    switch (R) {
    case (Reg8::A):
        f.SetCarry(a & 0x01);

        a = (a >> 1) | (a << 7);

        f.SetZero(!a);
        break;
    case (Reg8::B):
        f.SetCarry(b & 0x01);

        b = (b >> 1) | (b << 7);

        f.SetZero(!b);
        break;
    case (Reg8::C):
        f.SetCarry(c & 0x01);

        c = (c >> 1) | (c << 7);

        f.SetZero(!c);
        break;
    case (Reg8::D):
        f.SetCarry(d & 0x01);

        d = (d >> 1) | (d << 7);

        f.SetZero(!d);
        break;
    case (Reg8::E):
        f.SetCarry(e & 0x01);

        e = (e >> 1) | (e << 7);

        f.SetZero(!e);
        break;
    case (Reg8::H):
        f.SetCarry(h & 0x01);

        h = (h >> 1) | (h << 7);

        f.SetZero(!h);
        break;
    case (Reg8::L):
        f.SetCarry(l & 0x01);

        l = (l >> 1) | (l << 7);

        f.SetZero(!l);
        break;
    default:
        // Unreachable
        break;
    }
    f.SetSub(false);
    f.SetHalf(false);
}

void CPU::RotateRightMemAtHL() {
    u8 val = ReadMemAtHL();
    f.SetCarry(val & 0x01);

    val = (val >> 1) | (val << 7);

    f.SetZero(!val);
    f.SetSub(false);
    f.SetHalf(false);

    WriteMemAtHL(val);
}

void CPU::RotateRightThroughCarry(Reg8 R) {
    u8 carry_val = 0x00;
    switch (R) {
    case (Reg8::A):
        carry_val = a & 0x01;

        a = (a >> 1) | (f.Carry() << 7);

        f.SetZero(!a);
        break;
    case (Reg8::B):
        carry_val = b & 0x01;

        b = (b >> 1) | (f.Carry() << 7);

        f.SetZero(!b);
        break;
    case (Reg8::C):
        carry_val = c & 0x01;

        c = (c >> 1) | (f.Carry() << 7);

        f.SetZero(!c);
        break;
    case (Reg8::D):
        carry_val = d & 0x01;

        d = (d >> 1) | (f.Carry() << 7);

        f.SetZero(!d);
        break;
    case (Reg8::E):
        carry_val = e & 0x01;

        e = (e >> 1) | (f.Carry() << 7);

        f.SetZero(!e);
        break;
    case (Reg8::H):
        carry_val = h & 0x01;

        h = (h >> 1) | (f.Carry() << 7);

        f.SetZero(!h);
        break;
    case (Reg8::L):
        carry_val = l & 0x01;

        l = (l >> 1) | (f.Carry() << 7);

        f.SetZero(!l);
        break;
    default:
        // Unreachable
        break;
    }
    f.SetSub(false);
    f.SetHalf(false);
    f.SetCarry(carry_val);
}

void CPU::RotateRightMemAtHLThroughCarry() {
    u8 val = ReadMemAtHL();
    u8 carry_val = val & 0x01;

    val = (val >> 1) | (f.Carry() << 7);

    f.SetZero(!val);
    f.SetSub(false);
    f.SetHalf(false);
    f.SetCarry(carry_val);

    WriteMemAtHL(val);
}

void CPU::ShiftLeft(Reg8 R) {
    switch (R) {
    case (Reg8::A):
        f.SetCarry(a & 0x80);

        a <<= 1;

        f.SetZero(!a);
        break;
    case (Reg8::B):
        f.SetCarry(b & 0x80);

        b <<= 1;

        f.SetZero(!b);
        break;
    case (Reg8::C):
        f.SetCarry(c & 0x80);

        c <<= 1;

        f.SetZero(!c);
        break;
    case (Reg8::D):
        f.SetCarry(d & 0x80);

        d <<= 1;

        f.SetZero(!d);
        break;
    case (Reg8::E):
        f.SetCarry(e & 0x80);

        e <<= 1;

        f.SetZero(!e);
        break;
    case (Reg8::H):
        f.SetCarry(h & 0x80);

        h <<= 1;

        f.SetZero(!h);
        break;
    case (Reg8::L):
        f.SetCarry(l & 0x80);

        l <<= 1;

        f.SetZero(!l);
        break;
    default:
        // Unreachable
        break;
    }
    f.SetSub(false);
    f.SetHalf(false);
}

void CPU::ShiftLeftMemAtHL() {
    u8 val = ReadMemAtHL();
    f.SetCarry(val & 0x80);

    val <<= 1;

    f.SetZero(!val);
    f.SetSub(false);
    f.SetHalf(false);

    WriteMemAtHL(val);
}

void CPU::ShiftRightArithmetic(Reg8 R) {
    switch (R) {
    case (Reg8::A):
        f.SetCarry(a & 0x01);

        a >>= 1;
        a |= (a & 0x40) << 1;

        f.SetZero(!a);
        break;
    case (Reg8::B):
        f.SetCarry(b & 0x01);

        b >>= 1;
        b |= (b & 0x40) << 1;

        f.SetZero(!b);
        break;
    case (Reg8::C):
        f.SetCarry(c & 0x01);

        c >>= 1;
        c |= (c & 0x40) << 1;

        f.SetZero(!c);
        break;
    case (Reg8::D):
        f.SetCarry(d & 0x01);

        d >>= 1;
        d |= (d & 0x40) << 1;

        f.SetZero(!d);
        break;
    case (Reg8::E):
        f.SetCarry(e & 0x01);

        e >>= 1;
        e |= (e & 0x40) << 1;

        f.SetZero(!e);
        break;
    case (Reg8::H):
        f.SetCarry(h & 0x01);

        h >>= 1;
        h |= (h & 0x40) << 1;

        f.SetZero(!h);
        break;
    case (Reg8::L):
        f.SetCarry(l & 0x01);

        l >>= 1;
        l |= (l & 0x40) << 1;

        f.SetZero(!l);
        break;
    default:
        // Unreachable
        break;
    }
    f.SetSub(false);
    f.SetHalf(false);
}

void CPU::ShiftRightArithmeticMemAtHL() {
    u8 val = ReadMemAtHL();
    f.SetCarry(val & 0x01);

    val >>= 1;
    val |= (val & 0x40) << 1;

    f.SetZero(!val);
    f.SetSub(false);
    f.SetHalf(false);

    WriteMemAtHL(val);
}

void CPU::ShiftRightLogical(Reg8 R) {
    switch (R) {
    case (Reg8::A):
        f.SetCarry(a & 0x01);

        a >>= 1;

        f.SetZero(!a);
        break;
    case (Reg8::B):
        f.SetCarry(b & 0x01);

        b >>= 1;

        f.SetZero(!b);
        break;
    case (Reg8::C):
        f.SetCarry(c & 0x01);

        c >>= 1;

        f.SetZero(!c);
        break;
    case (Reg8::D):
        f.SetCarry(d & 0x01);

        d >>= 1;

        f.SetZero(!d);
        break;
    case (Reg8::E):
        f.SetCarry(e & 0x01);

        e >>= 1;

        f.SetZero(!e);
        break;
    case (Reg8::H):
        f.SetCarry(h & 0x01);

        h >>= 1;

        f.SetZero(!h);
        break;
    case (Reg8::L):
        f.SetCarry(l & 0x01);

        l >>= 1;

        f.SetZero(!l);
        break;
    default:
        // Unreachable
        break;
    }
    f.SetSub(false);
    f.SetHalf(false);
}

void CPU::ShiftRightLogicalMemAtHL() {
    u8 val = ReadMemAtHL();
    f.SetCarry(val & 0x01);

    val >>= 1;

    f.SetZero(!val);
    f.SetSub(false);
    f.SetHalf(false);

    WriteMemAtHL(val);
}

void CPU::SwapNybbles(Reg8 R) {
    switch (R) {
    case (Reg8::A):
        a = (a << 4) | (a >> 4);

        f.SetZero(!a);
        break;
    case (Reg8::B):
        b = (b << 4) | (b >> 4);

        f.SetZero(!b);
        break;
    case (Reg8::C):
        c = (c << 4) | (c >> 4);

        f.SetZero(!c);
        break;
    case (Reg8::D):
        d = (d << 4) | (d >> 4);

        f.SetZero(!d);
        break;
    case (Reg8::E):
        e = (e << 4) | (e >> 4);

        f.SetZero(!e);
        break;
    case (Reg8::H):
        h = (h << 4) | (h >> 4);

        f.SetZero(!h);
        break;
    case (Reg8::L):
        l = (l << 4) | (l >> 4);

        f.SetZero(!l);
        break;
    default:
        // Unreachable
        break;
    }
    f.SetSub(false);
    f.SetHalf(false);
    f.SetCarry(false);
}

void CPU::SwapMemAtHL() {
    u8 val = ReadMemAtHL();

    val = (val << 4) | (val >> 4);

    f.SetZero(!val);
    f.SetSub(false);
    f.SetHalf(false);
    f.SetCarry(false);

    WriteMemAtHL(val);
}

// Bit manipulation
void CPU::TestBit(unsigned int bit, u8 immediate) {
    f.SetZero(!(immediate & (0x01 << bit)));
    f.SetSub(false);
    f.SetHalf(true);
}

void CPU::TestBitOfMemAtHL(unsigned int bit) {
    f.SetZero(!(ReadMemAtHL() & (0x01 << bit)));
    f.SetSub(false);
    f.SetHalf(true);
}

void CPU::ResetBit(unsigned int bit, Reg8 R) {
    Write8(R, Read8(R) & ~(0x01 << bit)); // TODO: Clean up
}

void CPU::ResetBitOfMemAtHL(unsigned int bit) {
    u8 val = ReadMemAtHL();

    val &= ~(0x01 << bit);

    WriteMemAtHL(val);
}

void CPU::SetBit(unsigned int bit, Reg8 R) {
    Write8(R, Read8(R) | (0x01 << bit)); // TODO: Clean up
}

void CPU::SetBitOfMemAtHL(unsigned int bit) {
    u8 val = ReadMemAtHL();

    val |= (0x01 << bit);

    WriteMemAtHL(val);
}

// Jumps
void CPU::Jump(u16 addr) {
    // Internal delay
    gameboy->HardwareTick(4);

    pc = addr;
}

void CPU::JumpToHL() {
    pc = (static_cast<u16>(h) << 8) | static_cast<u16>(l);
}

void CPU::RelativeJump(s8 immediate) {
    // Internal delay
    gameboy->HardwareTick(4);

    pc += immediate;
}

// Calls and Returns
void CPU::Call(u16 addr) {
    // Internal delay
    gameboy->HardwareTick(4);

    mem.WriteMem8(--sp, static_cast<u8>(pc >> 8));
    gameboy->HardwareTick(4);

    mem.WriteMem8(--sp, static_cast<u8>(pc));
    gameboy->HardwareTick(4);

    pc = addr;
}

void CPU::Return() {
    // Internal delay
    gameboy->HardwareTick(4);

    u8 byte_lo = mem.ReadMem8(sp++);
    gameboy->HardwareTick(4);
    u8 byte_hi = mem.ReadMem8(sp++);
    gameboy->HardwareTick(4);

    pc = (static_cast<u16>(byte_hi) << 8) | static_cast<u16>(byte_lo);
}

// System Control
void CPU::Halt() {
    if (!interrupt_master_enable && mem.RequestedEnabledInterrupts()) {
        // If interrupts are disabled and there are requested, enabled interrupts pending when HALT is executed,
        // the GB will not enter halt mode. Instead, the GB will fail to increase the PC when executing the next 
        // instruction, thus executing it twice.
        cpu_mode = CPUMode::HaltBug;
    } else {
        cpu_mode = CPUMode::Halted;
    }
}

void CPU::Stop() {
    // STOP is a two-byte long opcode but only takes 4 cycles. If the opcode following STOP is not 0x00, the LCD
    // supposedly turns on.
    ++pc;

    // If the LCD is off, on pre-CGB devices it stays off, and on post-CGB devices it displays a black screen.
    // If the LCD is on, it stays on and the framebuffer is set to all white. Not really sure how this works, I guess
    // the LCD runs but BG, Window, and Sprites are all disabled and all LCD interrupts are ignored. I won't bother
    // emulating LCD behaviour during STOP until I understand how it works.

    // During STOP mode, the clock increases as usual, but normal interrupts are not serviced or checked. Regardless
    // if the joypad interrupt is enabled in the IE register, a stopped Game Boy will intercept any joypad presses
    // if the corresponding input lines in the P1 register are enabled.

    // Check if we should begin a speed switch.
    if (mem.game_mode == GameMode::CGB && mem.ReadMem8(0xFF4D) & 0x01) {
        // If the Game Boy receives an enabled joypad input during a speed switch, it will hang. Otherwise, it
        // returns to normal operation once the speed switch is complete.

        // A speed switch takes 128*1024-80=130992 cycles to complete, plus 4 cycles to decode the STOP instruction.
        speed_switch_cycles = 130992;
    } else if ((mem.ReadMem8(0xFF00) & 0x30) == 0x30) {
        throw std::runtime_error("The CPU has hung. Reason: STOP mode was entered with all joypad inputs disabled.");
    }

    cpu_mode = CPUMode::Stopped;
}

} // End namespace Core
