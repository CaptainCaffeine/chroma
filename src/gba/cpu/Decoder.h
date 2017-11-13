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

#pragma once

#include <vector>

#include "common/CommonTypes.h"
#include "gba/cpu/Instruction.h"

namespace Gba {

class Decoder {
public:
    bool DecodeThumb(Thumb opcode) const {
        for (const auto& instr : ThumbInstructions) {
            if (instr.Match(opcode)) {
                return true;
            }
        }

        return false;
    }

    bool DecodeArm(Arm opcode) const {
        for (const auto& instr : ArmInstructions) {
            if (instr.Match(opcode)) {
                return true;
            }
        }

        return false;
    }

private:
    const std::vector<Instruction<Thumb>> ThumbInstructions {
        {"ADC Rdn, Rm",           "0100000101mmmddd"},

        {"ADD Rd, Rn, #imm",      "0001110iiinnnddd"},
        {"ADD Rdn, #imm",         "00110dddiiiiiiii"},
        {"ADD Rd, Rn, Rm",        "0001100mmmnnnddd"},
        {"ADD Rdn, Rm",           "01000100dmmmmddd"}, // At least one of Rd, Rm not in R0-R7
        {"ADD Rd, SP, #imm",      "10101dddiiiiiiii"},
        {"ADD SP, SP, #imm",      "101100000iiiiiii"},
        {"ADD Rdm, SP, Rdm",      "01000100d1101ddd"}, // Any different from regular ADD Rdn, Rm?
        {"ADD SP, Rm",            "010001001mmmm101"}, // Any different from regular ADD Rdn, Rm?

        {"ADR Rd, label",         "10100dddiiiiiiii"},

        {"AND Rdn, Rm",           "0100000000mmmddd"},

        {"ASR Rd, Rm, #imm",      "00010iiiiimmmddd"},
        {"ASR Rdn, Rm",           "0100000100mmmddd"},

        {"B<c> label",            "1101cccciiiiiiii"},
        {"B label",               "11100iiiiiiiiiii"},

        {"BIC Rdn, Rm",           "0100001110mmmddd"},

        {"BL<c> label",           "11110iiiiiiiiiii"}, // H1, must be followed by H2.
        {"BL<c> label",           "11111iiiiiiiiiii"}, // H2, must be preceded by H1.

        {"BX<c> Rm",              "010001110mmmm000"},

        {"CMN Rn, Rm",            "0100001011mmmnnn"},

        {"CMP Rn, #imm",          "00101nnniiiiiiii"},
        {"CMP Rn, Rm",            "0100001010mmmnnn"}, // Rn and Rm both from R0-R7
        {"CMP Rn, Rm",            "01000101nmmmmnnn"}, // Rn and Rm both not from R0-R7

        {"EOR Rdn, Rm",           "0100000001mmmddd"},

        {"LDM{!} Rn, rlist",      "11001nnnrrrrrrrr"}, // ! if Rn not in rlist. Also LDMIA, LDMFD

        {"LDR Rt, [Rn, {#imm}]",  "01101iiiiinnnttt"},
        {"LDR Rt, [SP, {#imm}]",  "10011tttiiiiiiii"},
        {"LDR Rt, label",         "01001tttiiiiiiii"},
        {"LDR Rt, [Rn, Rm]",      "0101100mmmnnnttt"},

        {"LDRB Rt, [Rn, {#imm}]", "01111iiiiinnnttt"},
        {"LDRB Rt, [Rn, Rm]",     "0101110mmmnnnttt"},

        {"LDRH Rt, [Rn, {#imm}]", "10001iiiiinnnttt"},
        {"LDRH Rt, [Rn, Rm]",     "0101101mmmnnnttt"},

        {"LDRSB Rt, [Rn, Rm]",    "0101011mmmnnnttt"},

        {"LDRSH Rt, [Rn, Rm]",    "0101111mmmnnnttt"},

        {"LSL Rd, Rm, #imm",      "00000iiiiimmmddd"},
        {"LSL Rdn, Rm",           "0100000010mmmddd"},

        {"LSR Rd, Rm, #imm",      "00001iiiiimmmddd"},
        {"LSR Rdn, Rm",           "0100000011mmmddd"},

        {"MOV Rd, #imm",          "00100dddiiiiiiii"},
        {"MOV Rd, Rm",            "01000110dmmmmddd"}, // At least one of Rd, Rm not in R0-R7
        {"MOV Rd, Rm",            "0000000000mmmddd"},

        {"MUL Rdn, Rm",           "0100001101mmmddd"},

        {"MVN Rdn, Rm",           "0100001111mmmddd"},

        {"ORR Rdn, Rm",           "0100001100mmmddd"},

        {"POP rlist",             "1011110prrrrrrrr"},

        {"PUSH rlist",            "1011010mrrrrrrrr"},

        {"ROR Rdn, Rm",           "0100000111mmmddd"},

        {"RSB Rdn, Rm, #0",       "0100001001mmmddd"},

        {"SBC Rdn, Rm",           "0100000110mmmddd"},

        {"STM Rn!, rlist",        "11000nnnrrrrrrrr"}, // Also STMIA, STMEA

        {"STR Rt, [Rn, {#imm}]",  "01100iiiiinnnttt"},
        {"STR Rt, [SP, {#imm}]",  "10010tttiiiiiiii"},
        {"STR Rt, [Rn, Rm]",      "0101000mmmnnnttt"},

        {"STRB Rt, [Rn, {#imm}]", "01110iiiiinnnttt"},
        {"STRB Rt, [Rn, Rm]",     "0101010mmmnnnttt"},

        {"STRH Rt, [Rn, {#imm}]", "10000iiiiinnnttt"},
        {"STRH Rt, [Rn, Rm]",     "0101001mmmnnnttt"},

        {"SUB Rd, Rn, #imm",      "0001111iiinnnddd"},
        {"SUB Rdn, #imm",         "00111dddiiiiiiii"},
        {"SUB Rd, Rn, Rm",        "0001101mmmnnnddd"},
        //{"ADD Rd, SP, #imm",      "10101dddiiiiiiii"}, // No SUB form present for this in ARMv4T? Seems weird.
        {"SUB SP, SP, #imm",      "101100001iiiiiii"},

        {"SWI #imm",              "11011111iiiiiiii"}, // Called SVC in newer ARM verions.

        {"TST Rn, Rm",            "0100001000mmmnnn"},
    };

    const std::vector<Instruction<Arm>> ArmInstructions {
        {"ADC Rd, Rn, #imm",                 "cccc0010101Snnnnddddiiiiiiiiiiii"},
        {"ADC Rd, Rn, Rm, {shift}",          "cccc0000101Snnnnddddiiiiiqq0mmmm"},
        {"ADC Rd, Rn, Rm, type, Rs",         "cccc0000101Snnnnddddssss0qq1mmmm"},

        {"ADD Rd, Rn, #imm",                 "cccc0010100Snnnnddddiiiiiiiiiiii"},
        {"ADD Rd, Rn, Rm, {shift}",          "cccc0000100Snnnnddddiiiiiqq0mmmm"},
        {"ADD Rd, Rn, Rm, type, Rs",         "cccc0000100Snnnnddddssss0qq1mmmm"},
        {"ADD Rd, SP, #imm",                 "cccc0010100S1101ddddiiiiiiiiiiii"}, // Different from regular ADD Rd, Rn, imm?
        {"ADD Rd, SP, Rm, {shift}",          "cccc0000100S1101ddddiiiiiqq0mmmm"}, // Different from regular ADD Rd, Rn, Rm?

        {"ADR Rd, label",                    "cccc001010001111ddddiiiiiiiiiiii"}, // Label after current instruction.
        {"ADR Rd, label",                    "cccc001001001111ddddiiiiiiiiiiii"}, // Label before current instruction.

        {"AND Rd, Rn, #imm",                 "cccc0010000Snnnnddddiiiiiiiiiiii"},
        {"AND Rd, Rn, Rm, {shift}",          "cccc0000000Snnnnddddiiiiiqq0mmmm"},
        {"AND Rd, Rn, Rm, type, Rs",         "cccc0000000Snnnnddddssss0qq1mmmm"},

        {"ASR Rd, Rm, #imm",                 "cccc0001101S0000ddddiiiii100mmmm"}, // The Rn field is zeroed out in brackets.
        {"ASR Rd, Rn, Rm",                   "cccc0001101S0000ddddmmmm0101nnnn"}, // Just an alias for MOV?

        {"B label",                          "cccc1010iiiiiiiiiiiiiiiiiiiiiiii"},

        {"BIC Rd, Rn, #imm",                 "cccc0011110Snnnnddddiiiiiiiiiiii"},
        {"BIC Rd, Rn, Rm, {shift}",          "cccc0001110Snnnnddddiiiiiqq0mmmm"},
        {"BIC Rd, Rn, Rm, type, Rs",         "cccc0001110Snnnnddddssss0qq1mmmm"},

        {"BL label",                         "cccc1011iiiiiiiiiiiiiiiiiiiiiiii"},

        {"BX Rm",                            "cccc000100101111111111110001mmmm"},

        {"CDP coproc, opc1, CRd, CRn, CRm, opc2", "cccc1110oooonnnnddddkkkkppp0mmmm"},

        {"CMN Rn, #imm",                     "cccc00110111nnnn0000iiiiiiiiiiii"},
        {"CMN Rn, Rm, {shift}",              "cccc00010111nnnn0000iiiiiqq0mmmm"},
        {"CMN Rn, Rm, type, Rs",             "cccc00010111nnnn0000ssss0qq1mmmm"},

        {"CMP Rn, #imm",                     "cccc00110101nnnn0000iiiiiiiiiiii"},
        {"CMP Rn, Rm, {shift}",              "cccc00010101nnnn0000iiiiiqq0mmmm"},
        {"CMP Rn, Rm, type, Rs",             "cccc00010101nnnn0000ssss0qq1mmmm"},

        {"EOR Rd, Rn, #imm",                 "cccc0010001Snnnnddddiiiiiiiiiiii"},
        {"EOR Rd, Rn, Rm, {shift}",          "cccc0000001Snnnnddddiiiiiqq0mmmm"},
        {"EOR Rd, Rn, Rm, type, Rs",         "cccc0000001Snnnnddddssss0qq1mmmm"},

        {"LDC coproc, CRd, [Rn, #+/-imm]{!}","cccc110pudw1nnnnddddkkkkiiiiiiii"}, // Coproc instruction.
        {"LDC coproc, CRd, label",           "cccc110pudw11111ddddkkkkiiiiiiii"}, // PC-relative case of above.

        {"LDM Rn{!}, rlist",                 "cccc100010w1nnnnrrrrrrrrrrrrrrrr"}, // Also LDMIA, LDMFD
        {"LDMDA Rn{!}, rlist",               "cccc100000w1nnnnrrrrrrrrrrrrrrrr"}, // Also LDMFA
        {"LDMDB Rn{!}, rlist",               "cccc100100w1nnnnrrrrrrrrrrrrrrrr"}, // Also LDMEA
        {"LDMIB Rn{!}, rlist",               "cccc100110w1nnnnrrrrrrrrrrrrrrrr"}, // Also LDMED

        {"LDM{mode} Rn, rlist^",             "cccc100pu1w1nnnn1rrrrrrrrrrrrrrr"}, // Registers must contain PC.
        {"LDM{mode} Rn, rlist^",             "cccc100pu101nnnn0rrrrrrrrrrrrrrr"}, // Registers cannot contain PC. Cannot be executed from User or System modes.

        // Disassembly different for post-indexed (p == 0).
        {"LDR Rt, [Rn, {#+/-imm}]{!}",       "cccc010pu0w1nnnnttttiiiiiiiiiiii"},
        {"LDR Rt, label",                    "cccc0101u0011111ttttiiiiiiiiiiii"}, // PC-relative case of above.
        {"LDR Rt, [Rn, +/-Rm, {shift}]{!}",  "cccc011pu0w1nnnnttttiiiiiqq0mmmm"},

        {"LDRT Rt, [Rn], #+/-imm",           "cccc0100u011nnnnttttiiiiiiiiiiii"},
        {"LDRT Rt, [Rn], +/-Rm, {shift}",    "cccc0110u011nnnnttttiiiiiqq0mmmm"},

        {"LDRB Rt, [Rn, {#+/-imm}]{!}",      "cccc010pu1w1nnnnttttiiiiiiiiiiii"},
        {"LDRB Rt, label",                   "cccc0101u1011111ttttiiiiiiiiiiii"}, // PC-relative case of above.
        {"LDRB Rt, [Rn, +/-Rm, {shift}]{!}", "cccc011pu1w1nnnnttttiiiiiqq0mmmm"},

        {"LDRBT Rt, [Rn], #+/-imm",          "cccc0100u111nnnnttttiiiiiiiiiiii"},
        {"LDRBT Rt, [Rn], +/-Rm, {shift}",   "cccc0110u111nnnnttttiiiiiqq0mmmm"},

        {"LDRH Rt, [Rn, {#+/-imm}]{!}",      "cccc000pu1w1nnnnttttiiii1011iiii"},
        {"LDRH Rt, label",                   "cccc0001u1011111ttttiiii1011iiii"}, // PC-relative case of above.
        {"LDRH Rt, [Rn, +/-Rm]{!}",          "cccc000pu0w1nnnntttt00001011mmmm"},

        {"LDRSB Rt, [Rn, {#+/-imm}]{!}",     "cccc000pu1w1nnnnttttiiii1101iiii"},
        {"LDRSB Rt, label",                  "cccc0001u1011111ttttiiii1101iiii"}, // PC-relative case of above.
        {"LDRSB Rt, [Rn, +/-Rm]{!}",         "cccc000pu0w1nnnntttt00001101mmmm"},

        {"LDRSH Rt, [Rn, {#+/-imm}]{!}",     "cccc000pu1w1nnnnttttiiii1111iiii"},
        {"LDRSH Rt, label",                  "cccc0001u1011111ttttiiii1111iiii"}, // PC-relative case of above.
        {"LDRSH Rt, [Rn, +/-Rm]{!}",         "cccc000pu0w1nnnntttt00001111mmmm"},

        {"LSL Rd, Rm, #imm",                 "cccc0001101S0000ddddiiiii000mmmm"},
        {"LSL Rd, Rm, Rs",                   "cccc0001101S0000ddddssss0001mmmm"},

        {"LSR Rd, Rm, #imm",                 "cccc0001101S0000ddddiiiii010mmmm"},
        {"LSR Rd, Rm, Rs",                   "cccc0001101S0000ddddssss0011mmmm"},

        {"MCR coproc, opc1, Rt, CRn, CRm, opc2",  "cccc1110ooo0nnnnttttkkkkppp1mmmm"},

        {"MLA Rd, Rn, Rm, Ra",               "cccc0000001Sddddaaaammmm1001nnnn"},

        {"MOV Rd, #imm",                     "cccc0011101S0000ddddiiiiiiiiiiii"},
        {"MOV Rd, Rm",                       "cccc0001101S0000dddd00000000mmmm"},

        // Absorb all shift & rotate instructions into this one?
        // {"MOV Rd, Rm, type, shift",          "cccc0001101S0000ddddiiiiiqq0mmmm"},
        // {"MOV Rd, Rm, type, Rs",             "cccc0001101S0000ddddssss0qq1mmmm"},

        {"MRC coproc, opc1, Rt, CRn, CRm, opc2",  "cccc1110ooo1nnnnttttkkkkppp1mmmm"},

        {"MRS Rd, special_reg",              "cccc000100001111dddd000000000000"}, // Unprivileged only.
        {"MRS Rd, special_reg",              "cccc00010r001111dddd000000000000"}, // Privileged only.

        {"MSR special_reg, #imm",            "cccc00110010mm001111iiiiiiiiiiii"}, // Unprivileged only.
        {"MSR special_reg, Rn",              "cccc00010010mm00111100000000nnnn"}, // Unprivileged only.

        {"MSR special_reg, #imm",            "cccc00110r10mmmm1111iiiiiiiiiiii"}, // Privileged only.
        {"MSR special_reg, Rn",              "cccc00010r10mmmm111100000000nnnn"}, // Privileged only.

        {"MUL Rd, Rn, Rm",                   "cccc0000000Sdddd0000mmmm1001nnnn"},

        {"MVN Rd, #imm",                     "cccc0011111S0000ddddiiiiiiiiiiii"},
        {"MVN Rd, Rm, {shift}",              "cccc0001111S0000ddddiiiiiqq0mmmm"},
        {"MVN Rd, Rm, type, Rs",             "cccc0001111S0000ddddssss0qq1mmmm"},

        {"ORR Rd, Rn, #imm",                 "cccc0011100Snnnnddddiiiiiiiiiiii"},
        {"ORR Rd, Rn, Rm, {shift}",          "cccc0001100Snnnnddddiiiiiqq0mmmm"},
        {"ORR Rd, Rn, Rm, type, Rs",         "cccc0001100Snnnnddddssss0qq1mmmm"},

        {"POP rlist",                        "cccc100010111101rrrrrrrrrrrrrrrr"},
        {"POP Rt",                           "cccc010010011101tttt000000000100"},

        {"PUSH rlist",                       "cccc100100101101rrrrrrrrrrrrrrrr"},
        {"PUSH Rt",                          "cccc010100101101tttt000000000100"},

        {"ROR Rd, Rm, #imm",                 "cccc0001101S0000ddddiiiii110mmmm"},
        {"ROR Rd, Rm, Rs",                   "cccc0001101S0000ddddssss0111mmmm"},

        {"RRX Rd, Rm",                       "cccc0001101S0000dddd00000110mmmm"},

        {"RSB Rd, Rn, #imm",                 "cccc0010011Snnnnddddiiiiiiiiiiii"},
        {"RSB Rd, Rn, Rm, {shift}",          "cccc0000011Snnnnddddiiiiiqq0mmmm"},
        {"RSB Rd, Rn, Rm, type, Rs",         "cccc0000011Snnnnddddssss0qq1mmmm"},

        {"RSC Rd, Rn, #imm",                 "cccc0010111Snnnnddddiiiiiiiiiiii"},
        {"RSC Rd, Rn, Rm, {shift}",          "cccc0000111Snnnnddddiiiiiqq0mmmm"},
        {"RSC Rd, Rn, Rm, type, Rs",         "cccc0000111Snnnnddddssss0qq1mmmm"},

        {"SBC Rd, Rn, #imm",                 "cccc0010110Snnnnddddiiiiiiiiiiii"},
        {"SBC Rd, Rn, Rm, {shift}",          "cccc0000110Snnnnddddiiiiiqq0mmmm"},
        {"SBC Rd, Rn, Rm, type, Rs",         "cccc0000110Snnnnddddssss0qq1mmmm"},

        {"SMLAL RdLo, RdHi, Rn, Rm",         "cccc0000111Shhhhllllmmmm1001nnnn"},

        {"SMULL RdLo, RdHi, Rn, Rm",         "cccc0000110Shhhhllllmmmm1001nnnn"},

        {"STC coproc, CRd, [Rn, #+/-imm]{!}","cccc110pudw0nnnnddddkkkkiiiiiiii"}, // Coproc instruction.

        {"STM Rn{!}, rlist",                 "cccc100010w0nnnnrrrrrrrrrrrrrrrr"}, // Also STMIA, STMEA
        {"STMDA Rn{!}, rlist",               "cccc100000w0nnnnrrrrrrrrrrrrrrrr"}, // Also STMED
        {"STMDB Rn{!}, rlist",               "cccc100100w0nnnnrrrrrrrrrrrrrrrr"}, // Also STMFD
        {"STMIB Rn{!}, rlist",               "cccc100110w0nnnnrrrrrrrrrrrrrrrr"}, // Also STMFA

        {"STM{mode} Rn, rlist^",             "cccc100pu100nnnnrrrrrrrrrrrrrrrr"}, // Stores user-mode regs, cannot be executed from User or System modes.

        // Disassembly different for post-indexed (p == 0).
        {"STR Rt, [Rn, {#+/-imm}]{!}",       "cccc010pu0w0nnnnttttiiiiiiiiiiii"},
        {"STR Rt, [Rn, +/-Rm, {shift}]{!}",  "cccc011pu0w0nnnnttttiiiiiqq0mmmm"},

        {"STRT Rt, [Rn], {#+/-imm}",         "cccc0100u010nnnnttttiiiiiiiiiiii"},
        {"STRT Rt, [Rn], +/-Rm, {shift}",    "cccc0110u010nnnnttttiiiiiqq0mmmm"},

        {"STRB Rt, [Rn, {#+/-imm}]{!}",      "cccc010pu1w0nnnnttttiiiiiiiiiiii"},
        {"STRB Rt, [Rn, +/-Rm, {shift}]{!}", "cccc011pu1w0nnnnttttiiiiiqq0mmmm"},

        {"STRBT Rt, [Rn], #+/-imm",          "cccc0100u110nnnnttttiiiiiiiiiiii"},
        {"STRBT Rt, [Rn], +/-Rm, {shift}",   "cccc0110u110nnnnttttiiiiiqq0mmmm"},

        {"STRH Rt, [Rn, {#+/-imm}]{!}",      "cccc000pu1w0nnnnttttiiii1011iiii"},
        {"STRH Rt, [Rn, +/-Rm]{!}",          "cccc000pu0w0nnnntttt00001011mmmm"},

        {"SUB Rd, Rn, #imm",                 "cccc0010010Snnnnddddiiiiiiiiiiii"},
        {"SUB Rd, Rn, Rm, {shift}",          "cccc0000010Snnnnddddiiiiiqq0mmmm"},
        {"SUB Rd, Rn, Rm, type, Rs",         "cccc0000010Snnnnddddssss0qq1mmmm"},
        {"SUB Rd, SP, #imm",                 "cccc0010010S1101ddddiiiiiiiiiiii"}, // Different from regular SUB Rd, Rn, imm?
        {"SUB Rd, SP, Rm, {shift}",          "cccc0000010S1101ddddiiiiiqq0mmmm"}, // Different from regular SUB Rd, Rn, Rm?

        {"SWI #imm",                         "cccc1111iiiiiiiiiiiiiiiiiiiiiiii"}, // Called SVC in newer ARM versions.

        {"SWP{B} Rt, Rm, [Rn]",              "cccc00010b00nnnntttt00001001mmmm"},

        {"TEQ Rn, #imm",                     "cccc00110011nnnn0000iiiiiiiiiiii"},
        {"TEQ Rn, Rm, {shift}",              "cccc00010011nnnn0000iiiiiqq0mmmm"},
        {"TEQ Rn, Rm, type, Rs",             "cccc00010011nnnn0000ssss0qq1mmmm"},

        {"TST Rn, #imm",                     "cccc00110001nnnn0000iiiiiiiiiiii"},
        {"TST Rn, Rm, {shift}",              "cccc00010001nnnn0000iiiiiqq0mmmm"},
        {"TST Rn, Rm, type, Rs",             "cccc00010001nnnn0000ssss0qq1mmmm"},

        {"UMLAL RdLo, RdHi, Rn, Rm",         "cccc0000101Shhhhllllmmmm1001nnnn"},

        {"UMULL RdLo, RdHi, Rn, Rm",         "cccc0000100Shhhhllllmmmm1001nnnn"},
    };
};

} // End namespace Gba
