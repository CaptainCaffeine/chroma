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

#include "gba/cpu/Instruction.h"
#include "gba/cpu/Cpu.h"
#include "gba/cpu/Disassembler.h"

namespace Gba {

template<>
template<typename Dispatcher>
std::vector<Instruction<Thumb>> Instruction<Thumb>::GetInstructionTable() {
    std::vector<Instruction<Thumb>> thumb_instructions {
        {"0100000101mmmddd", &Dispatcher::Thumb_AdcReg},     // ADCS Rdn, Rm

        {"0001110iiinnnddd", &Dispatcher::Thumb_AddImmT1},   // ADDS Rd, Rn, #imm
        {"00110dddiiiiiiii", &Dispatcher::Thumb_AddImmT2},   // ADDS Rdn, #imm
        {"0001100mmmnnnddd", &Dispatcher::Thumb_AddRegT1},   // ADDS Rd, Rn, Rm
        {"01000100dmmmmddd", &Dispatcher::Thumb_AddRegT2},   // ADD Rdn, Rm
        {"10101dddiiiiiiii", &Dispatcher::Thumb_AddSpImmT1}, // ADD Rd, SP, #imm
        {"101100000iiiiiii", &Dispatcher::Thumb_AddSpImmT2}, // ADD SP, SP, #imm
        {"10100dddiiiiiiii", &Dispatcher::Thumb_AddPcImm},   // ADD Rd, PC, #imm

        {"0100000000mmmddd", &Dispatcher::Thumb_AndReg},     // ANDS Rdn, Rm

        {"00010iiiiimmmddd", &Dispatcher::Thumb_AsrImm},     // ASRS Rd, Rm, #imm
        {"0100000100mmmddd", &Dispatcher::Thumb_AsrReg},     // ASRS Rdn, Rm

        {"1101cccciiiiiiii", &Dispatcher::Thumb_BT1},        // B<c> label
        {"11100iiiiiiiiiii", &Dispatcher::Thumb_BT2},        // B label

        {"0100001110mmmddd", &Dispatcher::Thumb_BicReg},     // BICS Rdn, Rm

        {"11110iiiiiiiiiii", &Dispatcher::Thumb_BlH1},       // BL<c> label
        {"11111iiiiiiiiiii", &Dispatcher::Thumb_BlH2},       // BL<c> label

        {"010001110mmmm000", &Dispatcher::Thumb_Bx},         // BX Rm

        {"0100001011mmmnnn", &Dispatcher::Thumb_CmnReg},     // CMN Rn, Rm

        {"00101nnniiiiiiii", &Dispatcher::Thumb_CmpImm},     // CMP Rn, #imm
        {"0100001010mmmnnn", &Dispatcher::Thumb_CmpRegT1},   // CMP Rn, Rm
        {"01000101nmmmmnnn", &Dispatcher::Thumb_CmpRegT2},   // CMP Rn, Rm

        {"0100000001mmmddd", &Dispatcher::Thumb_EorReg},     // EORS Rdn, Rm

        {"11001nnnrrrrrrrr", &Dispatcher::Thumb_Ldm},        // LDM Rn{!}, rlist

        {"01101iiiiinnnttt", &Dispatcher::Thumb_LdrImm},     // LDR Rt, [Rn, {#imm}]
        {"10011tttiiiiiiii", &Dispatcher::Thumb_LdrSpImm},   // LDR Rt, [SP, {#imm}]
        {"01001tttiiiiiiii", &Dispatcher::Thumb_LdrPcImm},   // LDR Rt, [PC, #imm]; Normally "LDR Rt, label".
        {"0101100mmmnnnttt", &Dispatcher::Thumb_LdrReg},     // LDR Rt, [Rn, Rm]

        {"01111iiiiinnnttt", &Dispatcher::Thumb_LdrbImm},    // LDRB Rt, [Rn, {#imm}]
        {"0101110mmmnnnttt", &Dispatcher::Thumb_LdrbReg},    // LDRB Rt, [Rn, Rm]

        {"10001iiiiinnnttt", &Dispatcher::Thumb_LdrhImm},    // LDRH Rt, [Rn, {#imm}]
        {"0101101mmmnnnttt", &Dispatcher::Thumb_LdrhReg},    // LDRH Rt, [Rn, Rm]

        {"0101011mmmnnnttt", &Dispatcher::Thumb_LdrsbReg},   // LDRSB Rt, [Rn, Rm]
        {"0101111mmmnnnttt", &Dispatcher::Thumb_LdrshReg},   // LDRSH Rt, [Rn, Rm]

        {"00000iiiiimmmddd", &Dispatcher::Thumb_LslImm},     // LSLS Rd, Rm, #imm
        {"0100000010mmmddd", &Dispatcher::Thumb_LslReg},     // LSLS Rdn, Rm

        {"00001iiiiimmmddd", &Dispatcher::Thumb_LsrImm},     // LSRS Rd, Rm, #imm
        {"0100000011mmmddd", &Dispatcher::Thumb_LsrReg},     // LSRS Rdn, Rm

        {"00100dddiiiiiiii", &Dispatcher::Thumb_MovImm},     // MOVS Rd, #imm
        {"01000110dmmmmddd", &Dispatcher::Thumb_MovRegT1},   // MOV Rd, Rm
        {"0000000000mmmddd", &Dispatcher::Thumb_MovRegT2},   // MOVS Rd, Rm

        {"0100001101nnnddd", &Dispatcher::Thumb_MulReg},     // MULS Rdn, Rm

        {"0100001111mmmddd", &Dispatcher::Thumb_MvnReg},     // MVNS Rdn, Rm

        {"0100001100mmmddd", &Dispatcher::Thumb_OrrReg},     // ORRS Rdn, Rm

        {"1011110prrrrrrrr", &Dispatcher::Thumb_Pop},        // POP rlist

        {"1011010mrrrrrrrr", &Dispatcher::Thumb_Push},       // PUSH rlist

        {"0100000111mmmddd", &Dispatcher::Thumb_RorReg},     // RORS Rdn, Rm

        {"0100001001nnnddd", &Dispatcher::Thumb_RsbImm},     // RSBS Rdn, Rm, #0

        {"0100000110mmmddd", &Dispatcher::Thumb_SbcReg},     // SBCS Rdn, Rm

        {"11000nnnrrrrrrrr", &Dispatcher::Thumb_Stm},        // STM Rn!, rlist

        {"01100iiiiinnnttt", &Dispatcher::Thumb_StrImm},     // STR Rt, [Rn, {#imm}]
        {"10010tttiiiiiiii", &Dispatcher::Thumb_StrSpImm},   // STR Rt, [SP, {#imm}]
        {"0101000mmmnnnttt", &Dispatcher::Thumb_StrReg},     // STR Rt, [Rn, Rm]

        {"01110iiiiinnnttt", &Dispatcher::Thumb_StrbImm},    // STRB Rt, [Rn, {#imm}]
        {"0101010mmmnnnttt", &Dispatcher::Thumb_StrbReg},    // STRB Rt, [Rn, Rm]

        {"10000iiiiinnnttt", &Dispatcher::Thumb_StrhImm},    // STRH Rt, [Rn, {#imm}]
        {"0101001mmmnnnttt", &Dispatcher::Thumb_StrhReg},    // STRH Rt, [Rn, Rm]

        {"0001111iiinnnddd", &Dispatcher::Thumb_SubImmT1},   // SUBS Rd, Rn, #imm
        {"00111dddiiiiiiii", &Dispatcher::Thumb_SubImmT2},   // SUBS Rdn, #imm
        {"0001101mmmnnnddd", &Dispatcher::Thumb_SubReg},     // SUBS Rd, Rn, Rm
        {"101100001iiiiiii", &Dispatcher::Thumb_SubSpImm},   // SUB SP, SP, #imm

        {"11011111iiiiiiii", &Dispatcher::Thumb_Swi},        // SWI #imm

        {"0100001000mmmnnn", &Dispatcher::Thumb_TstReg},     // TST Rn, Rm

        {"iiiiiiiiiiiiiiii", &Dispatcher::Thumb_Undefined},  // Undefined
    };

    std::sort(thumb_instructions.begin(), thumb_instructions.end(), [](const auto& instr1, const auto& instr2) {
        return Popcount(instr2.fixed_mask) < Popcount(instr1.fixed_mask);
    });

    return thumb_instructions;
}

template std::vector<Instruction<Thumb>> Instruction<Thumb>::GetInstructionTable<Cpu>();
template std::vector<Instruction<Thumb>> Instruction<Thumb>::GetInstructionTable<Disassembler>();

template<>
template<typename Dispatcher>
std::vector<Instruction<Arm>> Instruction<Arm>::GetInstructionTable() {
    std::vector<Instruction<Arm>> arm_instructions {
        {"cccc0010101Snnnnddddiiiiiiiiiiii", &Dispatcher::Arm_AdcImm},        // ADC Rd, Rn, #imm
        {"cccc0000101Snnnnddddiiiiiqq0mmmm", &Dispatcher::Arm_AdcReg},        // ADC Rd, Rn, Rm, {shift}
        {"cccc0000101Snnnnddddssss0qq1mmmm", &Dispatcher::Arm_AdcRegShifted}, // ADC Rd, Rn, Rm, type, Rs

        {"cccc0010100Snnnnddddiiiiiiiiiiii", &Dispatcher::Arm_AddImm},        // ADD Rd, Rn, #imm
        {"cccc0000100Snnnnddddiiiiiqq0mmmm", &Dispatcher::Arm_AddReg},        // ADD Rd, Rn, Rm, {shift}
        {"cccc0000100Snnnnddddssss0qq1mmmm", &Dispatcher::Arm_AddRegShifted}, // ADD Rd, Rn, Rm, type, Rs

        {"cccc0010000Snnnnddddiiiiiiiiiiii", &Dispatcher::Arm_AndImm},        // AND Rd, Rn, #imm
        {"cccc0000000Snnnnddddiiiiiqq0mmmm", &Dispatcher::Arm_AndReg},        // AND Rd, Rn, Rm, {shift}
        {"cccc0000000Snnnnddddssss0qq1mmmm", &Dispatcher::Arm_AndRegShifted}, // AND Rd, Rn, Rm, type, Rs

        {"cccc0001101S0000ddddiiiii100mmmm", &Dispatcher::Arm_AsrImm},        // ASR Rd, Rm, #imm
        {"cccc0001101S0000ddddmmmm0101nnnn", &Dispatcher::Arm_AsrReg},        // ASR Rd, Rn, Rm

        {"cccc1010iiiiiiiiiiiiiiiiiiiiiiii", &Dispatcher::Arm_B},             // B label

        {"cccc0011110Snnnnddddiiiiiiiiiiii", &Dispatcher::Arm_BicImm},        // BIC Rd, Rn, #imm
        {"cccc0001110Snnnnddddiiiiiqq0mmmm", &Dispatcher::Arm_BicReg},        // BIC Rd, Rn, Rm, {shift}
        {"cccc0001110Snnnnddddssss0qq1mmmm", &Dispatcher::Arm_BicRegShifted}, // BIC Rd, Rn, Rm, type, Rs

        {"cccc1011iiiiiiiiiiiiiiiiiiiiiiii", &Dispatcher::Arm_Bl},            // BL label

        {"cccc000100101111111111110001mmmm", &Dispatcher::Arm_Bx},            // BX Rm

        {"cccc1110oooonnnnddddkkkkppp0mmmm", &Dispatcher::Arm_Cdp},           // CDP coproc, opc1, CRd, CRn, CRm, opc2

        {"cccc00110111nnnn0000iiiiiiiiiiii", &Dispatcher::Arm_CmnImm},        // CMN Rn, #imm
        {"cccc00010111nnnn0000iiiiiqq0mmmm", &Dispatcher::Arm_CmnReg},        // CMN Rn, Rm, {shift}
        {"cccc00010111nnnn0000ssss0qq1mmmm", &Dispatcher::Arm_CmnRegShifted}, // CMN Rn, Rm, type, Rs

        {"cccc00110101nnnn0000iiiiiiiiiiii", &Dispatcher::Arm_CmpImm},        // CMP Rn, #imm
        {"cccc00010101nnnn0000iiiiiqq0mmmm", &Dispatcher::Arm_CmpReg},        // CMP Rn, Rm, {shift}
        {"cccc00010101nnnn0000ssss0qq1mmmm", &Dispatcher::Arm_CmpRegShifted}, // CMP Rn, Rm, type, Rs

        {"cccc0010001Snnnnddddiiiiiiiiiiii", &Dispatcher::Arm_EorImm},        // EOR Rd, Rn, #imm
        {"cccc0000001Snnnnddddiiiiiqq0mmmm", &Dispatcher::Arm_EorReg},        // EOR Rd, Rn, Rm, {shift}
        {"cccc0000001Snnnnddddssss0qq1mmmm", &Dispatcher::Arm_EorRegShifted}, // EOR Rd, Rn, Rm, type, Rs

        {"cccc110pudw1nnnnddddkkkkiiiiiiii", &Dispatcher::Arm_Ldc},           // LDC coproc, CRd, [Rn, #+/-imm]{!}

        {"cccc100puew1nnnnrrrrrrrrrrrrrrrr", &Dispatcher::Arm_Ldm},           // LDM{U}{P} Rn{!}, rlist{^}

        {"cccc010pu0w1nnnnttttiiiiiiiiiiii", &Dispatcher::Arm_LdrImm},        // LDR Rt, [Rn, {#+/-imm}]{!}
        {"cccc011pu0w1nnnnttttiiiiiqq0mmmm", &Dispatcher::Arm_LdrReg},        // LDR Rt, [Rn, +/-Rm, {shift}]{!}

        {"cccc010pu1w1nnnnttttiiiiiiiiiiii", &Dispatcher::Arm_LdrbImm},       // LDRB Rt, [Rn, {#+/-imm}]{!}
        {"cccc011pu1w1nnnnttttiiiiiqq0mmmm", &Dispatcher::Arm_LdrbReg},       // LDRB Rt, [Rn, +/-Rm, {shift}]{!}

        {"cccc000pu1w1nnnnttttiiii1011iiii", &Dispatcher::Arm_LdrhImm},       // LDRH Rt, [Rn, {#+/-imm}]{!}
        {"cccc000pu0w1nnnntttt00001011mmmm", &Dispatcher::Arm_LdrhReg},       // LDRH Rt, [Rn, +/-Rm]{!}

        {"cccc000pu1w1nnnnttttiiii1101iiii", &Dispatcher::Arm_LdrsbImm},      // LDRSB Rt, [Rn, {#+/-imm}]{!}
        {"cccc000pu0w1nnnntttt00001101mmmm", &Dispatcher::Arm_LdrsbReg},      // LDRSB Rt, [Rn, +/-Rm]{!}

        {"cccc000pu1w1nnnnttttiiii1111iiii", &Dispatcher::Arm_LdrshImm},      // LDRSH Rt, [Rn, {#+/-imm}]{!}
        {"cccc000pu0w1nnnntttt00001111mmmm", &Dispatcher::Arm_LdrshReg},      // LDRSH Rt, [Rn, +/-Rm]{!}

        {"cccc0001101S0000ddddiiiii000mmmm", &Dispatcher::Arm_LslImm},        // LSL Rd, Rm, #imm
        {"cccc0001101S0000ddddmmmm0001nnnn", &Dispatcher::Arm_LslReg},        // LSL Rd, Rn, Rm

        {"cccc0001101S0000ddddiiiii010mmmm", &Dispatcher::Arm_LsrImm},        // LSR Rd, Rm, #imm
        {"cccc0001101S0000ddddmmmm0011nnnn", &Dispatcher::Arm_LsrReg},        // LSR Rd, Rn, Rm

        {"cccc1110ooo0nnnnttttkkkkppp1mmmm", &Dispatcher::Arm_Mcr},           // MCR coproc, opc1, Rt, CRn, CRm, opc2

        {"cccc0000001Sddddaaaammmm1001nnnn", &Dispatcher::Arm_MlaReg},        // MLA Rd, Rn, Rm, Ra

        {"cccc0011101S0000ddddiiiiiiiiiiii", &Dispatcher::Arm_MovImm},        // MOV Rd, #imm
        {"cccc0001101S0000dddd00000000mmmm", &Dispatcher::Arm_MovReg},        // MOV Rd, Rm

        {"cccc1110ooo1nnnnttttkkkkppp1mmmm", &Dispatcher::Arm_Mcr},           // MRC coproc, opc1, Rt, CRn, CRm, opc2

        {"cccc00010r001111dddd000000000000", &Dispatcher::Arm_Mrs},           // MRS Rd, special_reg

        {"cccc00110r10mmmm1111iiiiiiiiiiii", &Dispatcher::Arm_MsrImm},        // MSR special_reg, #imm
        {"cccc00010r10mmmm111100000000nnnn", &Dispatcher::Arm_MsrReg},        // MSR special_reg, Rn

        {"cccc0000000Sdddd0000mmmm1001nnnn", &Dispatcher::Arm_MulReg},        // MUL Rd, Rn, Rm

        {"cccc0011111S0000ddddiiiiiiiiiiii", &Dispatcher::Arm_MvnImm},        // MVN Rd, #imm
        {"cccc0001111S0000ddddiiiiiqq0mmmm", &Dispatcher::Arm_MvnReg},        // MVN Rd, Rm, {shift}
        {"cccc0001111S0000ddddssss0qq1mmmm", &Dispatcher::Arm_MvnRegShifted}, // MVN Rd, Rm, type, Rs

        {"cccc0011100Snnnnddddiiiiiiiiiiii", &Dispatcher::Arm_OrrImm},        // ORR Rd, Rn, #imm
        {"cccc0001100Snnnnddddiiiiiqq0mmmm", &Dispatcher::Arm_OrrReg},        // ORR Rd, Rn, Rm, {shift}
        {"cccc0001100Snnnnddddssss0qq1mmmm", &Dispatcher::Arm_OrrRegShifted}, // ORR Rd, Rn, Rm, type, Rs

        {"cccc100010111101rrrrrrrrrrrrrrrr", &Dispatcher::Arm_PopA1},         // POP rlist
        {"cccc010010011101tttt000000000100", &Dispatcher::Arm_PopA2},         // POP Rt

        {"cccc100100101101rrrrrrrrrrrrrrrr", &Dispatcher::Arm_PushA1},        // PUSH rlist
        {"cccc010100101101tttt000000000100", &Dispatcher::Arm_PushA2},        // PUSH Rt

        {"cccc0001101S0000ddddiiiii110mmmm", &Dispatcher::Arm_RorImm},        // ROR Rd, Rm, #imm; RRX if imm == 0
        {"cccc0001101S0000ddddmmmm0111nnnn", &Dispatcher::Arm_RorReg},        // ROR Rd, Rn, Rm

        {"cccc0010011Snnnnddddiiiiiiiiiiii", &Dispatcher::Arm_RsbImm},        // RSB Rd, Rn, #imm
        {"cccc0000011Snnnnddddiiiiiqq0mmmm", &Dispatcher::Arm_RsbReg},        // RSB Rd, Rn, Rm, {shift}
        {"cccc0000011Snnnnddddssss0qq1mmmm", &Dispatcher::Arm_RsbRegShifted}, // RSB Rd, Rn, Rm, type, Rs

        {"cccc0010111Snnnnddddiiiiiiiiiiii", &Dispatcher::Arm_RscImm},        // RSC Rd, Rn, #imm
        {"cccc0000111Snnnnddddiiiiiqq0mmmm", &Dispatcher::Arm_RscReg},        // RSC Rd, Rn, Rm, {shift}
        {"cccc0000111Snnnnddddssss0qq1mmmm", &Dispatcher::Arm_RscRegShifted}, // RSC Rd, Rn, Rm, type, Rs

        {"cccc0010110Snnnnddddiiiiiiiiiiii", &Dispatcher::Arm_SbcImm},        // SBC Rd, Rn, #imm
        {"cccc0000110Snnnnddddiiiiiqq0mmmm", &Dispatcher::Arm_SbcReg},        // SBC Rd, Rn, Rm, {shift}
        {"cccc0000110Snnnnddddssss0qq1mmmm", &Dispatcher::Arm_SbcRegShifted}, // SBC Rd, Rn, Rm, type, Rs

        {"cccc0000111Shhhhllllmmmm1001nnnn", &Dispatcher::Arm_SmlalReg},      // SMLAL RdLo, RdHi, Rn, Rm
        {"cccc0000110Shhhhllllmmmm1001nnnn", &Dispatcher::Arm_SmullReg},      // SMULL RdLo, RdHi, Rn, Rm

        {"cccc110pudw0nnnnddddkkkkiiiiiiii", &Dispatcher::Arm_Ldc},           // STC coproc, CRd, [Rn, #+/-imm]{!}

        {"cccc100puew0nnnnrrrrrrrrrrrrrrrr", &Dispatcher::Arm_Stm},           // STM{U}{P} Rn{!}, rlist{^}

        {"cccc010pu0w0nnnnttttiiiiiiiiiiii", &Dispatcher::Arm_StrImm},        // STR Rt, [Rn, {#+/-imm}]{!}
        {"cccc011pu0w0nnnnttttiiiiiqq0mmmm", &Dispatcher::Arm_StrReg},        // STR Rt, [Rn, +/-Rm, {shift}]{!}

        {"cccc010pu1w0nnnnttttiiiiiiiiiiii", &Dispatcher::Arm_StrbImm},       // STRB Rt, [Rn, {#+/-imm}]{!}
        {"cccc011pu1w0nnnnttttiiiiiqq0mmmm", &Dispatcher::Arm_StrbReg},       // STRB Rt, [Rn, +/-Rm, {shift}]{!}

        {"cccc000pu1w0nnnnttttiiii1011iiii", &Dispatcher::Arm_StrhImm},       // STRH Rt, [Rn, {#+/-imm}]{!}
        {"cccc000pu0w0nnnntttt00001011mmmm", &Dispatcher::Arm_StrhReg},       // STRH Rt, [Rn, +/-Rm]{!}

        {"cccc0010010Snnnnddddiiiiiiiiiiii", &Dispatcher::Arm_SubImm},        // SUB Rd, Rn, #imm
        {"cccc0000010Snnnnddddiiiiiqq0mmmm", &Dispatcher::Arm_SubReg},        // SUB Rd, Rn, Rm, {shift}
        {"cccc0000010Snnnnddddssss0qq1mmmm", &Dispatcher::Arm_SubRegShifted}, // SUB Rd, Rn, Rm, type, Rs

        {"cccc1111iiiiiiiiiiiiiiiiiiiiiiii", &Dispatcher::Arm_Swi},           // SWI #imm

        {"cccc00010b00nnnntttt00001001mmmm", &Dispatcher::Arm_SwpReg},        // SWP{B} Rt, Rm, [Rn]

        {"cccc00110011nnnn0000iiiiiiiiiiii", &Dispatcher::Arm_TeqImm},        // TEQ Rn, #imm
        {"cccc00010011nnnn0000iiiiiqq0mmmm", &Dispatcher::Arm_TeqReg},        // TEQ Rn, Rm, {shift}
        {"cccc00010011nnnn0000ssss0qq1mmmm", &Dispatcher::Arm_TeqRegShifted}, // TEQ Rn, Rm, type, Rs

        {"cccc00110001nnnn0000iiiiiiiiiiii", &Dispatcher::Arm_TstImm},        // TST Rn, #imm
        {"cccc00010001nnnn0000iiiiiqq0mmmm", &Dispatcher::Arm_TstReg},        // TST Rn, Rm, {shift}
        {"cccc00010001nnnn0000ssss0qq1mmmm", &Dispatcher::Arm_TstRegShifted}, // TST Rn, Rm, type, Rs

        {"cccc0000101Shhhhllllmmmm1001nnnn", &Dispatcher::Arm_UmlalReg},      // UMLAL RdLo, RdHi, Rn, Rm
        {"cccc0000100Shhhhllllmmmm1001nnnn", &Dispatcher::Arm_UmullReg},      // UMULL RdLo, RdHi, Rn, Rm

        {"iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii", &Dispatcher::Arm_Undefined},     // Undefined
    };

    std::sort(arm_instructions.begin(), arm_instructions.end(), [](const auto& instr1, const auto& instr2) {
        return Popcount(instr2.fixed_mask) < Popcount(instr1.fixed_mask);
    });

    return arm_instructions;
}

template std::vector<Instruction<Arm>> Instruction<Arm>::GetInstructionTable<Cpu>();
template std::vector<Instruction<Arm>> Instruction<Arm>::GetInstructionTable<Disassembler>();

} // End namespace Gba
