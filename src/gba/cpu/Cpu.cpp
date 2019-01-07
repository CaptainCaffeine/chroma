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

#include <algorithm>
#include <cassert>

#include "gba/cpu/Cpu.h"
#include "gba/cpu/Instruction.h"
#include "gba/cpu/Disassembler.h"
#include "gba/core/Core.h"
#include "gba/memory/Memory.h"
#include "gba/hardware/Dma.h"

namespace Gba {

Cpu::Cpu(Memory& _mem, Core& _core)
        : mem(_mem)
        , core(_core)
        , thumb_instructions(GetThumbInstructionTable<Cpu>())
        , arm_instructions(GetArmInstructionTable<Cpu>()) {

    PopulateThumbDecodeTable();
}

// Needed to declare std::vector with forward-declared type in the header file.
Cpu::~Cpu() = default;

int Cpu::Execute(int cycles) {
    while (cycles > 0) {
        int cycles_taken = 0;

        if (dma_active) {
            // The CPU is blocked while DMA is active.
            // Higher priority DMAs preempt lower priority ones, where DMA0 is the highest priority.
            for (auto& dma : core.dma) {
                if (dma.Active()) {
                    cycles_taken = dma.Run();
                    break;
                }
            }

            core.UpdateHardware(cycles_taken);
            cycles -= cycles_taken;
            continue;
        }

        if (mem.PendingInterrupts()) {
            if (halted) {
                halted = false;
                core.disasm->LogHalt();
            }

            if (InterruptsEnabled()) {
                cycles_taken += TakeException(CpuMode::Irq);
            }
        }

        if (halted) {
            const int halt_cycles = core.HaltCycles(cycles);
            core.UpdateHardware(halt_cycles);
            core.disasm->IncHaltCycles(halt_cycles);
            cycles -= halt_cycles;
            continue;
        }

        if (ThumbMode()) {
            pipeline[0] = pipeline[1];
            pipeline[1] = pipeline[2];
            pipeline[2] = mem.ReadMem<Thumb>(regs[pc]);
            cycles_taken += mem.AccessTime<Thumb>(regs[pc], AccessType::Opcode);

            // Sync hardware after the prefetch.
            core.UpdateHardware(cycles_taken);
            cycles -= cycles_taken;
            cycles_taken = 0;

            core.disasm->DisassembleThumb(pipeline[0], regs, cpsr);
            const auto& impl = DecodeThumb(pipeline[0]);
            cycles_taken += impl(*this, pipeline[0]);

            if (!pc_written) {
                // Only increment the PC if the executing instruction didn't change it.
                regs[pc] += 2;
            }
        } else {
            pipeline[0] = pipeline[1];
            pipeline[1] = pipeline[2];
            pipeline[2] = mem.ReadMem<Arm>(regs[pc]);
            cycles_taken += mem.AccessTime<Arm>(regs[pc], AccessType::Opcode);

            // Sync hardware after the prefetch.
            core.UpdateHardware(cycles_taken);
            cycles -= cycles_taken;
            cycles_taken = 0;

            core.disasm->DisassembleArm(pipeline[0], regs, cpsr);
            const auto& impl = DecodeArm(pipeline[0]);
            cycles_taken += impl(*this, pipeline[0]);

            if (!pc_written) {
                // Only increment the PC if the executing instruction didn't change it.
                regs[pc] += 4;
            }
        }

        // Sync hardware again after the rest of the instruction has executed.
        if (cycles_taken != 0) {
            core.UpdateHardware(cycles_taken);
            cycles -= cycles_taken;
        }

        pc_written = false;
    }

    // Return the number of overspent cycles.
    return cycles;
}

void Cpu::PopulateThumbDecodeTable() {
    // The lower 6 bits of all Thumb opcodes are variable, so we only need to use the top 10 bits to identify
    // which instruction implementation to use.
    for (u16 opcode = 0; opcode < 0x400; ++opcode) {
        for (const auto& instr : thumb_instructions) {
            if (instr.Match(opcode << 6)) {
                thumb_decode_table[opcode] = &instr.impl_func;
                break;
            }
        }
    }
}

const std::function<int(Cpu& cpu, Thumb opcode)>& Cpu::DecodeThumb(Thumb opcode) const {
    return *thumb_decode_table[opcode >> 6];
}

const std::function<int(Cpu& cpu, Arm opcode)>& Cpu::DecodeArm(Arm opcode) {
    int opcode_hash = ((opcode >> 16) ^ opcode) * 0x45D9F3B;
    opcode_hash = ((opcode_hash >> 16) ^ opcode_hash) * 0x45D9F3B;
    opcode_hash = ((opcode_hash >> 16) ^ opcode_hash) % arm_decode_cache.size();

    auto& cache_entries = arm_decode_cache[opcode_hash];
    for (const auto& instr : cache_entries) {
        if (instr->Match(opcode)) {
            return instr->impl_func;
        }
    }

    // Opcode not in cache.
    for (const auto& instr : arm_instructions) {
        if (instr.Match(opcode)) {
            cache_entries.push_back(&instr);
            return instr.impl_func;
        }
    }

    // Undefined instruction.
    return arm_instructions.back().impl_func;
}

bool Cpu::InterruptsEnabled() const {
    return !(cpsr & irq_disable) && mem.InterruptMasterEnable();
}

bool Cpu::ValidCpuMode(u32 new_mode) const {
    switch (static_cast<CpuMode>(new_mode & cpu_mode)) {
    case CpuMode::User:
    case CpuMode::Fiq:
    case CpuMode::Irq:
    case CpuMode::Svc:
    case CpuMode::Abort:
    case CpuMode::Undef:
    case CpuMode::System:
        return true;
    default:
        return false;
    }
}

void Cpu::CpuModeSwitch(CpuMode new_cpu_mode) {
    sp_banked[CurrentCpuModeIndex()] = regs[sp];
    lr_banked[CurrentCpuModeIndex()] = regs[lr];

    regs[sp] = sp_banked[CpuModeIndex(new_cpu_mode)];
    regs[lr] = lr_banked[CpuModeIndex(new_cpu_mode)];

    // Swap R8-R12 with the banked values if switched to or from FIQ mode, unless we're "switching" from FIQ to FIQ.
    if ((new_cpu_mode == CpuMode::Fiq || CurrentCpuMode() == CpuMode::Fiq) && new_cpu_mode != CurrentCpuMode()) {
        std::swap_ranges(regs.begin() + 8, regs.begin() + 13, fiq_banked_regs.begin());
    }

    cpsr = (cpsr & ~cpu_mode) | static_cast<u32>(new_cpu_mode);
}

int Cpu::FlushPipeline() {
    mem.FlushPrefetchBuffer();

    int cycles = 0;
    if (ThumbMode()) {
        pipeline[1] = mem.ReadMem<Thumb>(regs[pc]);
        cycles += mem.AccessTime<Thumb>(regs[pc], AccessType::Opcode);
        regs[pc] += 2;

        pipeline[2] = mem.ReadMem<Thumb>(regs[pc]);
        cycles += mem.AccessTime<Thumb>(regs[pc], AccessType::Opcode);
        regs[pc] += 2;
    } else {
        pipeline[1] = mem.ReadMem<Arm>(regs[pc]);
        cycles += mem.AccessTime<Arm>(regs[pc], AccessType::Opcode);
        regs[pc] += 4;

        pipeline[2] = mem.ReadMem<Arm>(regs[pc]);
        cycles += mem.AccessTime<Arm>(regs[pc], AccessType::Opcode);
        regs[pc] += 4;
    }

    pc_written = true;
    return cycles;
}

int Cpu::TakeException(CpuMode exception_type) {
    // Save current CPSR and switch to the new CPU mode.
    spsr[CpuModeIndex(exception_type)] = cpsr;
    CpuModeSwitch(exception_type);

    // Update the LR with the correct value for the exception type.
    // For IRQ, LR is the address of the instruction on which the IRQ occurred, plus 4.
    // For SVC and Undefined, LR is the address of the instruction after the one that caused the exception.
    if (ThumbMode()) {
        if (exception_type == CpuMode::Irq) {
            regs[lr] = regs[pc];
        } else {
            regs[lr] = regs[pc] - 2;
        }
    } else {
        regs[lr] = regs[pc] - 4;
    }

    // Disable IRQs and switch to ARM mode.
    cpsr |= irq_disable;
    cpsr &= ~thumb_mode;

    int cycles = 0;
    // Jump to the exception vector.
    switch (exception_type) {
    case CpuMode::Undef:
        regs[pc] = 0x04;
        // Undefined instruction exceptions take an extra internal cycle.
        cycles = 1;
        break;
    case CpuMode::Svc:
        regs[pc] = 0x08;
        break;
    case CpuMode::Irq:
        regs[pc] = 0x18;
        last_bios_fetch = 0xE25EF004;
        break;
    default:
        // There are no abort or FIQ exceptions on the GBA.
        assert(false);
        break;
    }

    return cycles + FlushPipeline();
}

int Cpu::ReturnFromException(u32 address) {
    if (CurrentCpuMode() == CpuMode::Irq) {
        last_bios_fetch = 0xE55EC002;
    } else if (CurrentCpuMode() == CpuMode::Svc) {
        last_bios_fetch = 0xE3A02004;
    }

    u32 spsr_exception = spsr[CurrentCpuModeIndex()];
    CpuModeSwitch(static_cast<CpuMode>(spsr_exception & cpu_mode));
    cpsr = spsr_exception;

    if (ThumbMode()) {
        return Thumb_BranchWritePC(address);
    } else {
        return Arm_BranchWritePC(address);
    }
}

void Cpu::LoadInternalCycle(int cycles) {
    if (regs[pc] >= BaseAddr::Rom) {
        if (mem.PrefetchEnabled()) {
            if (core.mem->LastAccessWasInRom()) {
                // Bug: the next access is non-sequential if the load was from ROM. Possibly related to the
                // prefetch disable bug.
                mem.MakeNextAccessNonsequential();
            } else {
                mem.RunPrefetch(cycles);
                mem.MakeNextAccessSequential(regs[pc]);
            }
        } else {
            // Prefetch disable bug: make the next access non-sequential.
            mem.MakeNextAccessNonsequential();
        }
    } else {
        // Make the next access sequential. This only has an effect for LDR/LDM/SWP opcodes, in which case the next
        // opcode fetch is a sequential access, despite the data load. It could be that the I-cycle gives it the
        // extra time to decode the next expected opcode address.
        mem.MakeNextAccessSequential(regs[pc]);
    }
}

void Cpu::InternalCycle(int cycles) {
    if (regs[pc] >= BaseAddr::Rom) {
        if (mem.PrefetchEnabled()) {
            mem.RunPrefetch(cycles);
        } else {
            // Prefetch disable bug: make the next access non-sequential.
            mem.MakeNextAccessNonsequential();
        }
    }
}

void Cpu::StorePrefetch() {
    // If STR/STM/PUSH is executed while prefetch is enabled, the next opcode fetch will be a sequential access
    // instead of non-sequential.
    if (mem.PrefetchEnabled()) {
        mem.MakeNextAccessSequential(regs[pc]);
    }
}

int Cpu::BxWritePC(u32 addr) {
    if ((addr & 0x1) == 0x1) {
        // Switch to Thumb mode.
        cpsr |= thumb_mode;
        regs[pc] = addr & ~0x1;
    } else {
        // Switch to Arm mode.
        cpsr &= ~thumb_mode;
        regs[pc] = addr & ~0x3;
    }

    return FlushPipeline();
}

ImmediateShift Cpu::DecodeImmShift(ShiftType type, u32 imm5) {
    if (imm5 == 0) {
        switch (type) {
        case ShiftType::LSL:
            return {type, imm5};
        case ShiftType::LSR:
            return {type, 32};
        case ShiftType::ASR:
            return {type, 32};
        case ShiftType::ROR:
            return {ShiftType::RRX, 1};
        default:
            assert(false);
            return {ShiftType::LSL, 0};
        }
    }

    return {type, imm5};
}

u32 Cpu::Shift(u32 value, ShiftType type, int shift_amount) {
    assert(!(type == ShiftType::RRX && shift_amount != 1));

    if (shift_amount == 0) {
        return value;
    }

    // ARM allows specifying shift lengths of up to 255, of which all 32 and above behave the same. But on x86,
    // shifting by more than the bit-length of the value is a no-op, so we clamp the shift length to 32.
    // We need to cast to 64-bits before shifting so that shifting by 32 works correctly.
    switch (type) {
    case ShiftType::LSL:
        return static_cast<u64>(value) << std::min(shift_amount, 32);
    case ShiftType::LSR:
        return static_cast<u64>(value) >> std::min(shift_amount, 32);
    case ShiftType::ASR:
        return static_cast<s64>(static_cast<s32>(value)) >> std::min(shift_amount, 32);
    case ShiftType::ROR:
        return RotateRight(value, shift_amount);
    case ShiftType::RRX:
        return (value >> 1) | (GetCarry() << 31);
    default:
        assert(false);
        return 0;
    }
}

Cpu::ResultWithCarry Cpu::Shift_C(u32 value, ShiftType type, int shift_amount) {
    assert(!(type == ShiftType::RRX && shift_amount != 1));

    if (shift_amount == 0) {
        return {value, GetCarry()};
    }

    // Shifting by 32 will still update the carry flag normally, so all shift lengths 33 and above behave the same.
    switch (type) {
    case ShiftType::LSL:
        return LogicalShiftLeft_C(value, std::min(shift_amount, 33));
    case ShiftType::LSR:
        return LogicalShiftRight_C(value, std::min(shift_amount, 33));
    case ShiftType::ASR:
        return ArithmeticShiftRight_C(value, std::min(shift_amount, 33));
    case ShiftType::ROR:
        return RotateRight_C(value, shift_amount);
    case ShiftType::RRX:
        return RotateRightExtend_C(value, GetCarry());
    default:
        assert(false);
        return {0, 0};
    }
}

Cpu::ResultWithCarry Cpu::LogicalShiftLeft_C(u64 value, int shift_amount) {
    u32 carry_out = (value << (shift_amount - 1)) >> 31;
    u32 result = value << shift_amount;

    return {result, carry_out};
}

Cpu::ResultWithCarry Cpu::LogicalShiftRight_C(u64 value, int shift_amount) {
    u32 carry_out = (value >> (shift_amount - 1)) & 0x1;
    u32 result = value >> shift_amount;

    return {result, carry_out};
}

Cpu::ResultWithCarry Cpu::ArithmeticShiftRight_C(s32 value, int shift_amount) {
    u32 carry_out = (static_cast<s64>(value) >> (shift_amount - 1)) & 0x1;
    u32 result = static_cast<s64>(value) >> shift_amount;

    return {result, carry_out};
}

Cpu::ResultWithCarry Cpu::RotateRight_C(u32 value, int shift_amount) {
    u32 result = RotateRight(value, shift_amount);

    return {result, result >> 31};
}

Cpu::ResultWithCarry Cpu::RotateRightExtend_C(u32 value, u32 carry_in) {
    return {(value >> 1) | (carry_in << 31), value & 0x1};
}

bool Cpu::ConditionPassed(Condition cond) const {
    switch (cond) {
    case Condition::Equal:         return GetZero();
    case Condition::NotEqual:      return !GetZero();
    case Condition::CarrySet:      return GetCarry();
    case Condition::CarryClear:    return !GetCarry();
    case Condition::Minus:         return GetSign();
    case Condition::Plus:          return !GetSign();
    case Condition::OverflowSet:   return GetOverflow();
    case Condition::OverflowClear: return !GetOverflow();
    case Condition::Higher:        return GetCarry() && !GetZero();
    case Condition::LowerSame:     return !(GetCarry() && !GetZero());
    case Condition::GreaterEqual:  return GetSign() == GetOverflow();
    case Condition::LessThan:      return !(GetSign() == GetOverflow());
    case Condition::GreaterThan:   return GetSign() == GetOverflow() && !GetZero();
    case Condition::LessEqual:     return !(GetSign() == GetOverflow() && !GetZero());
    default:                       return true;
    }
}

void Cpu::SetAllFlags(ArithResult result) {
    SetSign(result.value & sign_bit);
    SetZero(static_cast<u32>(result.value) == 0);
    SetCarry(result.value & carry_bit);
    SetOverflow(result.overflow);
}

void Cpu::SetSignZeroCarryFlags(u32 result, u32 carry) {
    SetSign(result & sign_bit);
    SetZero(result == 0);
    SetCarry(carry);
}

void Cpu::SetSignZeroFlags(u32 result) {
    SetSign(result & sign_bit);
    SetZero(result == 0);
}

void Cpu::ConditionalSetAllFlags(bool set_flags, ArithResult result) {
    if (set_flags) {
        SetAllFlags(result);
    }
}

void Cpu::ConditionalSetSignZeroCarryFlags(bool set_flags, u32 result, u32 carry) {
    if (set_flags) {
        SetSignZeroCarryFlags(result, carry);
    }
}

void Cpu::ConditionalSetSignZeroFlags(bool set_flags, u32 result) {
    if (set_flags) {
        SetSignZeroFlags(result);
    }
}

void Cpu::ConditionalSetMultiplyLongFlags(bool set_flags, u64 result) {
    if (set_flags) {
        SetSign(result & (1l << 63));
        SetZero(result == 0);
        SetCarry(0);
        SetOverflow(0);
    }
}

int Cpu::MultiplyCycles(u32 operand) {
    u32 mask = 0xFFFFFF00;
    for (int cycles = 1; cycles < 4; ++cycles) {
        if ((operand & mask) == mask || (operand & mask) == 0) {
            return cycles;
        }

        mask <<= 8;
    }

    return 4;
}

} // End namespace Gba
