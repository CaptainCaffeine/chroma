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

#pragma once

#include <vector>
#include <string>
#include <functional>
#include <utility>

#include "common/CommonTypes.h"
#include "common/CommonFuncs.h"

namespace Gba {

class Cpu;
class Disassembler;

template<typename T>
class Instruction {
public:
    template<typename... Args>
    Instruction(const char* _disasm, const char* instr_layout, int(Cpu::* impl)(Args...))
            : disasm(_disasm) {

        auto fields = CreateMasks(instr_layout);
        impl_func = GetImplFunction(impl, fields, std::index_sequence_for<Args...>{});
    }

    template<typename... Args>
    Instruction(const char* _disasm, const char* instr_layout, std::string(Disassembler::* impl)(Args...))
            : disasm(_disasm) {

        auto fields = CreateMasks(instr_layout);
        disasm_func = GetImplFunction(impl, fields, std::index_sequence_for<Args...>{});
    }

    bool Match(T opcode) const {
        return (opcode & fixed_mask) == instr_mask;
    }

    template<typename Dispatcher>
    static std::vector<Instruction<T>> GetInstructionTable();

    std::function<int(Cpu& cpu, T opcode)> impl_func;
    std::function<std::string(Disassembler& dis, T opcode)> disasm_func;
private:
    static constexpr auto num_bits = sizeof(T) * 8;

    std::string disasm;

    T fixed_mask = 0;
    T instr_mask = 0;

    struct FieldMask {
        FieldMask(T _mask, int _shift) : mask(_mask), shift(_shift) {}

        T mask;
        int shift;
    };

    std::vector<FieldMask> CreateMasks(const std::string& instr_layout) {
        char last_bit = '0';
        T current_mask = 0;
        std::vector<FieldMask> fields;

        for (std::size_t i = 0; i < instr_layout.size(); ++i) {
            const char bit = instr_layout[i];
            const int shift = num_bits - 1 - i;
            const T bit_mask = 1 << shift;

            if (bit != last_bit && last_bit != '0' && last_bit != '1') {
                fields.emplace_back(current_mask, shift + 1);
                current_mask = 0;
            }

            if (bit == '1' || bit == '0') {
                fixed_mask |= bit_mask;

                if (bit == '1') {
                    instr_mask |= bit_mask;
                }
            } else {
                current_mask |= bit_mask;
            }

            last_bit = bit;
        }

        fields.emplace_back(current_mask, 0);

        return fields;
    }

    template<typename ReturnType, typename D, typename... Args, std::size_t... Is>
    auto GetImplFunction(ReturnType(D::* impl)(Args...), std::vector<FieldMask> fields, std::index_sequence<Is...>) {
        return [impl, fields](D& dis, T opcode) -> ReturnType {
            return (dis.*impl)(static_cast<Args>((opcode & fields[Is].mask) >> fields[Is].shift)...);
        };
    }
};

} // End namespace Gba
