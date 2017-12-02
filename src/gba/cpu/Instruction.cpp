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

#include <algorithm>

#include "gba/cpu/Instruction.h"
#include "gba/cpu/Cpu.h"

namespace Gba {

template<>
std::vector<Instruction<Thumb>> Instruction<Thumb>::GetInstructionTable() {
    std::vector<Instruction<Thumb>> thumb_instructions {
        {"ADCS Rdn, Rm",          "0100000101mmmddd", &Cpu::Thumb_AdcReg},

        {"ADDS Rd, Rn, #imm",     "0001110iiinnnddd", &Cpu::Thumb_AddImmT1},
        {"ADDS Rdn, #imm",        "00110dddiiiiiiii", &Cpu::Thumb_AddImmT2},
        {"ADDS Rd, Rn, Rm",       "0001100mmmnnnddd", &Cpu::Thumb_AddRegT1},
        {"ADD Rdn, Rm",           "01000100dmmmmddd", &Cpu::Thumb_AddRegT2},
        {"ADD Rd, SP, #imm",      "10101dddiiiiiiii", &Cpu::Thumb_AddSpImmT1},
        {"ADD SP, SP, #imm",      "101100000iiiiiii", &Cpu::Thumb_AddSpImmT2},
        {"ADD Rd, PC, #imm",      "10100dddiiiiiiii", &Cpu::Thumb_AddPcImm},

        {"ANDS Rdn, Rm",          "0100000000mmmddd", &Cpu::Thumb_AndReg},

        {"ASRS Rd, Rm, #imm",     "00010iiiiimmmddd", &Cpu::Thumb_AsrImm},
        {"ASRS Rdn, Rm",          "0100000100mmmddd", &Cpu::Thumb_AsrReg},

        {"B<c> label",            "1101cccciiiiiiii", &Cpu::Thumb_BT1},
        {"B label",               "11100iiiiiiiiiii", &Cpu::Thumb_BT2},

        {"BICS Rdn, Rm",          "0100001110mmmddd", &Cpu::Thumb_BicReg},

        {"BL<c> label",           "11110iiiiiiiiiii", &Cpu::Thumb_BlH1},
        {"BL<c> label",           "11111iiiiiiiiiii", &Cpu::Thumb_BlH2},

        {"BX<c> Rm",              "010001110mmmm000", &Cpu::Thumb_Bx},

        {"CMN Rn, Rm",            "0100001011mmmnnn", &Cpu::Thumb_CmnReg},

        {"CMP Rn, #imm",          "00101nnniiiiiiii", &Cpu::Thumb_CmpImm},
        {"CMP Rn, Rm",            "0100001010mmmnnn", &Cpu::Thumb_CmpRegT1},
        {"CMP Rn, Rm",            "01000101nmmmmnnn", &Cpu::Thumb_CmpRegT2},

        {"EORS Rdn, Rm",          "0100000001mmmddd", &Cpu::Thumb_EorReg},

        {"LDM{!} Rn, rlist",      "11001nnnrrrrrrrr", &Cpu::Thumb_Ldm}, // ! if Rn not in rlist. Also LDMIA, LDMFD.

        {"LDR Rt, [Rn, {#imm}]",  "01101iiiiinnnttt", &Cpu::Thumb_LdrImm},
        {"LDR Rt, [SP, {#imm}]",  "10011tttiiiiiiii", &Cpu::Thumb_LdrSpImm},
        {"LDR Rt, [PC, #imm]",    "01001tttiiiiiiii", &Cpu::Thumb_LdrPcImm}, // Normally "LDR Rt, label".
        {"LDR Rt, [Rn, Rm]",      "0101100mmmnnnttt", &Cpu::Thumb_LdrReg},

        {"LDRB Rt, [Rn, {#imm}]", "01111iiiiinnnttt", &Cpu::Thumb_LdrbImm},
        {"LDRB Rt, [Rn, Rm]",     "0101110mmmnnnttt", &Cpu::Thumb_LdrbReg},

        {"LDRH Rt, [Rn, {#imm}]", "10001iiiiinnnttt", &Cpu::Thumb_LdrhImm},
        {"LDRH Rt, [Rn, Rm]",     "0101101mmmnnnttt", &Cpu::Thumb_LdrhReg},

        {"LDRSB Rt, [Rn, Rm]",    "0101011mmmnnnttt", &Cpu::Thumb_LdrsbReg},
        {"LDRSH Rt, [Rn, Rm]",    "0101111mmmnnnttt", &Cpu::Thumb_LdrshReg},

        {"LSLS Rd, Rm, #imm",     "00000iiiiimmmddd", &Cpu::Thumb_LslImm},
        {"LSLS Rdn, Rm",          "0100000010mmmddd", &Cpu::Thumb_LslReg},

        {"LSRS Rd, Rm, #imm",     "00001iiiiimmmddd", &Cpu::Thumb_LsrImm},
        {"LSRS Rdn, Rm",          "0100000011mmmddd", &Cpu::Thumb_LsrReg},

        {"MOVS Rd, #imm",         "00100dddiiiiiiii", &Cpu::Thumb_MovImm},
        {"MOV Rd, Rm",            "01000110dmmmmddd", &Cpu::Thumb_MovRegT1},
        {"MOVS Rd, Rm",           "0000000000mmmddd", &Cpu::Thumb_MovRegT2},

        {"MULS Rdn, Rm",          "0100001101nnnddd", &Cpu::Thumb_MulReg},

        {"MVNS Rdn, Rm",          "0100001111mmmddd", &Cpu::Thumb_MvnReg},

        {"ORRS Rdn, Rm",          "0100001100mmmddd", &Cpu::Thumb_OrrReg},

        {"POP rlist",             "1011110prrrrrrrr", &Cpu::Thumb_Pop},

        {"PUSH rlist",            "1011010mrrrrrrrr", &Cpu::Thumb_Push},

        {"RORS Rdn, Rm",          "0100000111mmmddd", &Cpu::Thumb_RorReg},

        {"RSBS Rdn, Rm, #0",      "0100001001nnnddd", &Cpu::Thumb_RsbImm},

        {"SBCS Rdn, Rm",          "0100000110mmmddd", &Cpu::Thumb_SbcReg},

        {"STM Rn!, rlist",        "11000nnnrrrrrrrr", &Cpu::Thumb_Stm}, // Also STMIA, STMEA

        {"STR Rt, [Rn, {#imm}]",  "01100iiiiinnnttt", &Cpu::Thumb_StrImm},
        {"STR Rt, [SP, {#imm}]",  "10010tttiiiiiiii", &Cpu::Thumb_StrSpImm},
        {"STR Rt, [Rn, Rm]",      "0101000mmmnnnttt", &Cpu::Thumb_StrReg},

        {"STRB Rt, [Rn, {#imm}]", "01110iiiiinnnttt", &Cpu::Thumb_StrbImm},
        {"STRB Rt, [Rn, Rm]",     "0101010mmmnnnttt", &Cpu::Thumb_StrbReg},

        {"STRH Rt, [Rn, {#imm}]", "10000iiiiinnnttt", &Cpu::Thumb_StrhImm},
        {"STRH Rt, [Rn, Rm]",     "0101001mmmnnnttt", &Cpu::Thumb_StrhReg},

        {"SUBS Rd, Rn, #imm",     "0001111iiinnnddd", &Cpu::Thumb_SubImmT1},
        {"SUBS Rdn, #imm",        "00111dddiiiiiiii", &Cpu::Thumb_SubImmT2},
        {"SUBS Rd, Rn, Rm",       "0001101mmmnnnddd", &Cpu::Thumb_SubReg},
        {"SUB SP, SP, #imm",      "101100001iiiiiii", &Cpu::Thumb_SubSpImm},

        {"SWI #imm",              "11011111iiiiiiii", &Cpu::Thumb_Swi},

        {"TST Rn, Rm",            "0100001000mmmnnn", &Cpu::Thumb_TstReg},

        {"Undefined",             "iiiiiiiiiiiiiiii", &Cpu::Thumb_Undefined},
    };

    std::sort(thumb_instructions.begin(), thumb_instructions.end(), [](const auto& instr1, const auto& instr2) {
        return Popcount(instr2.fixed_mask) < Popcount(instr1.fixed_mask);
    });

    return thumb_instructions;
}

template<>
std::vector<Instruction<Arm>> Instruction<Arm>::GetInstructionTable() {
    std::vector<Instruction<Arm>> arm_instructions {
        {"ADC Rd, Rn, #imm",                 "cccc0010101Snnnnddddiiiiiiiiiiii", &Cpu::Arm_AdcImm},
        {"ADC Rd, Rn, Rm, {shift}",          "cccc0000101Snnnnddddiiiiiqq0mmmm", &Cpu::Arm_AdcReg},
        {"ADC Rd, Rn, Rm, type, Rs",         "cccc0000101Snnnnddddssss0qq1mmmm", &Cpu::Arm_AdcRegShifted},

        {"ADD Rd, Rn, #imm",                 "cccc0010100Snnnnddddiiiiiiiiiiii", &Cpu::Arm_AddImm},
        {"ADD Rd, Rn, Rm, {shift}",          "cccc0000100Snnnnddddiiiiiqq0mmmm", &Cpu::Arm_AddReg},
        {"ADD Rd, Rn, Rm, type, Rs",         "cccc0000100Snnnnddddssss0qq1mmmm", &Cpu::Arm_AddRegShifted},

        {"AND Rd, Rn, #imm",                 "cccc0010000Snnnnddddiiiiiiiiiiii", &Cpu::Arm_AndImm},
        {"AND Rd, Rn, Rm, {shift}",          "cccc0000000Snnnnddddiiiiiqq0mmmm", &Cpu::Arm_AndReg},
        {"AND Rd, Rn, Rm, type, Rs",         "cccc0000000Snnnnddddssss0qq1mmmm", &Cpu::Arm_AndRegShifted},

        {"ASR Rd, Rm, #imm",                 "cccc0001101S0000ddddiiiii100mmmm", &Cpu::Arm_AsrImm},
        {"ASR Rd, Rn, Rm",                   "cccc0001101S0000ddddmmmm0101nnnn", &Cpu::Arm_AsrReg},

        {"B label",                          "cccc1010iiiiiiiiiiiiiiiiiiiiiiii", &Cpu::Arm_B},

        {"BIC Rd, Rn, #imm",                 "cccc0011110Snnnnddddiiiiiiiiiiii", &Cpu::Arm_BicImm},
        {"BIC Rd, Rn, Rm, {shift}",          "cccc0001110Snnnnddddiiiiiqq0mmmm", &Cpu::Arm_BicReg},
        {"BIC Rd, Rn, Rm, type, Rs",         "cccc0001110Snnnnddddssss0qq1mmmm", &Cpu::Arm_BicRegShifted},

        {"BL label",                         "cccc1011iiiiiiiiiiiiiiiiiiiiiiii", &Cpu::Arm_Bl},

        {"BX Rm",                            "cccc000100101111111111110001mmmm", &Cpu::Arm_Bx},

        {"CDP coproc, opc1, CRd, CRn, CRm, opc2", "cccc1110oooonnnnddddkkkkppp0mmmm", &Cpu::Arm_Cdp},

        {"CMN Rn, #imm",                     "cccc00110111nnnn0000iiiiiiiiiiii", &Cpu::Arm_CmnImm},
        {"CMN Rn, Rm, {shift}",              "cccc00010111nnnn0000iiiiiqq0mmmm", &Cpu::Arm_CmnReg},
        {"CMN Rn, Rm, type, Rs",             "cccc00010111nnnn0000ssss0qq1mmmm", &Cpu::Arm_CmnRegShifted},

        {"CMP Rn, #imm",                     "cccc00110101nnnn0000iiiiiiiiiiii", &Cpu::Arm_CmpImm},
        {"CMP Rn, Rm, {shift}",              "cccc00010101nnnn0000iiiiiqq0mmmm", &Cpu::Arm_CmpReg},
        {"CMP Rn, Rm, type, Rs",             "cccc00010101nnnn0000ssss0qq1mmmm", &Cpu::Arm_CmpRegShifted},

        {"EOR Rd, Rn, #imm",                 "cccc0010001Snnnnddddiiiiiiiiiiii", &Cpu::Arm_EorImm},
        {"EOR Rd, Rn, Rm, {shift}",          "cccc0000001Snnnnddddiiiiiqq0mmmm", &Cpu::Arm_EorReg},
        {"EOR Rd, Rn, Rm, type, Rs",         "cccc0000001Snnnnddddssss0qq1mmmm", &Cpu::Arm_EorRegShifted},

        {"LDC coproc, CRd, [Rn, #+/-imm]{!}","cccc110pudw1nnnnddddkkkkiiiiiiii", &Cpu::Arm_Ldc},

        {"LDMI{P} Rn{!}, rlist",             "cccc100p10w1nnnnrrrrrrrrrrrrrrrr", &Cpu::Arm_Ldmi}, // LDM if p == 0
        {"LDMD{P} Rn{!}, rlist",             "cccc100p00w1nnnnrrrrrrrrrrrrrrrr", &Cpu::Arm_Ldmd},

        {"LDM{mode} Rn, rlist^",             "cccc100pu1w1nnnn1rrrrrrrrrrrrrrr", &Cpu::Arm_Undefined}, // Registers must contain PC.
        {"LDM{mode} Rn, rlist^",             "cccc100pu101nnnn0rrrrrrrrrrrrrrr", &Cpu::Arm_Undefined}, // Registers cannot contain PC. Cannot be executed from User or System modes.

        // Disassembly different for post-indexed (p == 0). Disassemble as LDR*T when p == 0 && w == 1.
        {"LDR Rt, [Rn, {#+/-imm}]{!}",       "cccc010pu0w1nnnnttttiiiiiiiiiiii", &Cpu::Arm_LdrImm},
        {"LDR Rt, [Rn, +/-Rm, {shift}]{!}",  "cccc011pu0w1nnnnttttiiiiiqq0mmmm", &Cpu::Arm_LdrReg},

        {"LDRB Rt, [Rn, {#+/-imm}]{!}",      "cccc010pu1w1nnnnttttiiiiiiiiiiii", &Cpu::Arm_LdrbImm},
        {"LDRB Rt, [Rn, +/-Rm, {shift}]{!}", "cccc011pu1w1nnnnttttiiiiiqq0mmmm", &Cpu::Arm_LdrbReg},

        {"LDRH Rt, [Rn, {#+/-imm}]{!}",      "cccc000pu1w1nnnnttttiiii1011iiii", &Cpu::Arm_LdrhImm},
        {"LDRH Rt, [Rn, +/-Rm]{!}",          "cccc000pu0w1nnnntttt00001011mmmm", &Cpu::Arm_LdrhReg},

        {"LDRSB Rt, [Rn, {#+/-imm}]{!}",     "cccc000pu1w1nnnnttttiiii1101iiii", &Cpu::Arm_LdrsbImm},
        {"LDRSB Rt, [Rn, +/-Rm]{!}",         "cccc000pu0w1nnnntttt00001101mmmm", &Cpu::Arm_LdrsbReg},

        {"LDRSH Rt, [Rn, {#+/-imm}]{!}",     "cccc000pu1w1nnnnttttiiii1111iiii", &Cpu::Arm_LdrshImm},
        {"LDRSH Rt, [Rn, +/-Rm]{!}",         "cccc000pu0w1nnnntttt00001111mmmm", &Cpu::Arm_LdrshReg},

        {"LSL Rd, Rm, #imm",                 "cccc0001101S0000ddddiiiii000mmmm", &Cpu::Arm_LslImm},
        {"LSL Rd, Rm, Rs",                   "cccc0001101S0000ddddmmmm0001nnnn", &Cpu::Arm_LslReg},

        {"LSR Rd, Rm, #imm",                 "cccc0001101S0000ddddiiiii010mmmm", &Cpu::Arm_LsrImm},
        {"LSR Rd, Rm, Rs",                   "cccc0001101S0000ddddmmmm0011nnnn", &Cpu::Arm_LsrReg},

        {"MCR coproc, opc1, Rt, CRn, CRm, opc2",  "cccc1110ooo0nnnnttttkkkkppp1mmmm", &Cpu::Arm_Mcr},

        {"MLA Rd, Rn, Rm, Ra",               "cccc0000001Sddddaaaammmm1001nnnn", &Cpu::Arm_MlaReg},

        {"MOV Rd, #imm",                     "cccc0011101S0000ddddiiiiiiiiiiii", &Cpu::Arm_MovImm},
        {"MOV Rd, Rm",                       "cccc0001101S0000dddd00000000mmmm", &Cpu::Arm_MovReg},

        {"MRC coproc, opc1, Rt, CRn, CRm, opc2",  "cccc1110ooo1nnnnttttkkkkppp1mmmm", &Cpu::Arm_Mcr},

        {"MRS Rd, special_reg",              "cccc00010r001111dddd000000000000", &Cpu::Arm_Mrs},

        {"MSR special_reg, #imm",            "cccc00110r10mmmm1111iiiiiiiiiiii", &Cpu::Arm_MsrImm},
        {"MSR special_reg, Rn",              "cccc00010r10mmmm111100000000nnnn", &Cpu::Arm_MsrReg},

        {"MUL Rd, Rn, Rm",                   "cccc0000000Sdddd0000mmmm1001nnnn", &Cpu::Arm_MulReg},

        {"MVN Rd, #imm",                     "cccc0011111S0000ddddiiiiiiiiiiii", &Cpu::Arm_MvnImm},
        {"MVN Rd, Rm, {shift}",              "cccc0001111S0000ddddiiiiiqq0mmmm", &Cpu::Arm_MvnReg},
        {"MVN Rd, Rm, type, Rs",             "cccc0001111S0000ddddssss0qq1mmmm", &Cpu::Arm_MvnRegShifted},

        {"ORR Rd, Rn, #imm",                 "cccc0011100Snnnnddddiiiiiiiiiiii", &Cpu::Arm_OrrImm},
        {"ORR Rd, Rn, Rm, {shift}",          "cccc0001100Snnnnddddiiiiiqq0mmmm", &Cpu::Arm_OrrReg},
        {"ORR Rd, Rn, Rm, type, Rs",         "cccc0001100Snnnnddddssss0qq1mmmm", &Cpu::Arm_OrrRegShifted},

        {"POP rlist",                        "cccc100010111101rrrrrrrrrrrrrrrr", &Cpu::Arm_PopA1},
        {"POP Rt",                           "cccc010010011101tttt000000000100", &Cpu::Arm_PopA2},

        {"PUSH rlist",                       "cccc100100101101rrrrrrrrrrrrrrrr", &Cpu::Arm_PushA1},
        {"PUSH Rt",                          "cccc010100101101tttt000000000100", &Cpu::Arm_PushA2},

        {"ROR Rd, Rm, #imm",                 "cccc0001101S0000ddddiiiii110mmmm", &Cpu::Arm_RorImm}, // RRX if imm == 0
        {"ROR Rd, Rm, Rs",                   "cccc0001101S0000ddddssss0111mmmm", &Cpu::Arm_RorReg},

        {"RSB Rd, Rn, #imm",                 "cccc0010011Snnnnddddiiiiiiiiiiii", &Cpu::Arm_RsbImm},
        {"RSB Rd, Rn, Rm, {shift}",          "cccc0000011Snnnnddddiiiiiqq0mmmm", &Cpu::Arm_RsbReg},
        {"RSB Rd, Rn, Rm, type, Rs",         "cccc0000011Snnnnddddssss0qq1mmmm", &Cpu::Arm_RsbRegShifted},

        {"RSC Rd, Rn, #imm",                 "cccc0010111Snnnnddddiiiiiiiiiiii", &Cpu::Arm_RscImm},
        {"RSC Rd, Rn, Rm, {shift}",          "cccc0000111Snnnnddddiiiiiqq0mmmm", &Cpu::Arm_RscReg},
        {"RSC Rd, Rn, Rm, type, Rs",         "cccc0000111Snnnnddddssss0qq1mmmm", &Cpu::Arm_RscRegShifted},

        {"SBC Rd, Rn, #imm",                 "cccc0010110Snnnnddddiiiiiiiiiiii", &Cpu::Arm_SbcImm},
        {"SBC Rd, Rn, Rm, {shift}",          "cccc0000110Snnnnddddiiiiiqq0mmmm", &Cpu::Arm_SbcReg},
        {"SBC Rd, Rn, Rm, type, Rs",         "cccc0000110Snnnnddddssss0qq1mmmm", &Cpu::Arm_SbcRegShifted},

        {"SMLAL RdLo, RdHi, Rn, Rm",         "cccc0000111Shhhhllllmmmm1001nnnn", &Cpu::Arm_SmlalReg},

        {"SMULL RdLo, RdHi, Rn, Rm",         "cccc0000110Shhhhllllmmmm1001nnnn", &Cpu::Arm_SmullReg},

        {"STC coproc, CRd, [Rn, #+/-imm]{!}","cccc110pudw0nnnnddddkkkkiiiiiiii", &Cpu::Arm_Ldc},

        {"STMI{P} Rn{!}, rlist",             "cccc100p10w0nnnnrrrrrrrrrrrrrrrr", &Cpu::Arm_Stmi}, // STM when p == 0
        {"STMD{P} Rn{!}, rlist",             "cccc100p00w0nnnnrrrrrrrrrrrrrrrr", &Cpu::Arm_Stmd},

        {"STM{mode} Rn, rlist^",             "cccc100pu100nnnnrrrrrrrrrrrrrrrr", &Cpu::Arm_Undefined}, // Stores user-mode regs, cannot be executed from User or System modes.

        // Disassembly different for post-indexed (p == 0).
        {"STR Rt, [Rn, {#+/-imm}]{!}",       "cccc010pu0w0nnnnttttiiiiiiiiiiii", &Cpu::Arm_StrImm},
        {"STR Rt, [Rn, +/-Rm, {shift}]{!}",  "cccc011pu0w0nnnnttttiiiiiqq0mmmm", &Cpu::Arm_StrReg},

        {"STRB Rt, [Rn, {#+/-imm}]{!}",      "cccc010pu1w0nnnnttttiiiiiiiiiiii", &Cpu::Arm_StrbImm},
        {"STRB Rt, [Rn, +/-Rm, {shift}]{!}", "cccc011pu1w0nnnnttttiiiiiqq0mmmm", &Cpu::Arm_StrbReg},

        {"STRH Rt, [Rn, {#+/-imm}]{!}",      "cccc000pu1w0nnnnttttiiii1011iiii", &Cpu::Arm_StrhImm},
        {"STRH Rt, [Rn, +/-Rm]{!}",          "cccc000pu0w0nnnntttt00001011mmmm", &Cpu::Arm_StrhReg},

        {"SUB Rd, Rn, #imm",                 "cccc0010010Snnnnddddiiiiiiiiiiii", &Cpu::Arm_SubImm},
        {"SUB Rd, Rn, Rm, {shift}",          "cccc0000010Snnnnddddiiiiiqq0mmmm", &Cpu::Arm_SubReg},
        {"SUB Rd, Rn, Rm, type, Rs",         "cccc0000010Snnnnddddssss0qq1mmmm", &Cpu::Arm_SubRegShifted},

        {"SWI #imm",                         "cccc1111iiiiiiiiiiiiiiiiiiiiiiii", &Cpu::Arm_Swi},

        {"SWP{B} Rt, Rm, [Rn]",              "cccc00010b00nnnntttt00001001mmmm", &Cpu::Arm_SwpReg},

        {"TEQ Rn, #imm",                     "cccc00110011nnnn0000iiiiiiiiiiii", &Cpu::Arm_TeqImm},
        {"TEQ Rn, Rm, {shift}",              "cccc00010011nnnn0000iiiiiqq0mmmm", &Cpu::Arm_TeqReg},
        {"TEQ Rn, Rm, type, Rs",             "cccc00010011nnnn0000ssss0qq1mmmm", &Cpu::Arm_TeqRegShifted},

        {"TST Rn, #imm",                     "cccc00110001nnnn0000iiiiiiiiiiii", &Cpu::Arm_TstImm},
        {"TST Rn, Rm, {shift}",              "cccc00010001nnnn0000iiiiiqq0mmmm", &Cpu::Arm_TstReg},
        {"TST Rn, Rm, type, Rs",             "cccc00010001nnnn0000ssss0qq1mmmm", &Cpu::Arm_TstRegShifted},

        {"UMLAL RdLo, RdHi, Rn, Rm",         "cccc0000101Shhhhllllmmmm1001nnnn", &Cpu::Arm_UmlalReg},

        {"UMULL RdLo, RdHi, Rn, Rm",         "cccc0000100Shhhhllllmmmm1001nnnn", &Cpu::Arm_UmullReg},

        {"Undefined",                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii", &Cpu::Arm_Undefined},
    };

    std::sort(arm_instructions.begin(), arm_instructions.end(), [](const auto& instr1, const auto& instr2) {
        return Popcount(instr2.fixed_mask) < Popcount(instr1.fixed_mask);
    });

    return arm_instructions;
}
} // End namespace Gba
