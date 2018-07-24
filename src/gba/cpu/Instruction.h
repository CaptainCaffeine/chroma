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
#include <array>
#include <string>
#include <functional>
#include <utility>

#include "common/CommonTypes.h"
#include "common/CommonFuncs.h"

namespace Gba {

class Cpu;
class Disassembler;

template<typename T, typename Dispatcher>
class Instruction {
public:
    template<typename... Args>
    Instruction(const char* instr_layout, typename Dispatcher::ReturnType(Dispatcher::* impl)(Args...)) {
        const auto fields = CreateMasks<sizeof...(Args)>(instr_layout);
        impl_func = GetImplFunction(impl, fields, std::index_sequence_for<Args...>{});
    }

    std::function<typename Dispatcher::ReturnType(Dispatcher& dis, T opcode)> impl_func;

    bool Match(T opcode) const { return (opcode & fixed_mask) == instr_mask; }
    std::size_t FixedMaskSize() const { return Popcount(fixed_mask); }

private:
    static constexpr auto num_bits = sizeof(T) * 8;

    T fixed_mask = 0;
    T instr_mask = 0;

    struct FieldMask {
        T mask;
        int shift;
    };

    template<std::size_t N>
    std::array<FieldMask, N> CreateMasks(const std::string& instr_layout) {
        char last_bit = '0';
        T current_mask = 0;
        std::array<FieldMask, N> fields;
        int field_index = 0;

        for (std::size_t i = 0; i < instr_layout.size(); ++i) {
            const char bit = instr_layout[i];
            const int shift = num_bits - 1 - i;
            const T bit_mask = 1 << shift;

            if (bit != last_bit && last_bit != '0' && last_bit != '1') {
                fields[field_index++] = {current_mask, shift + 1};
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

        if (current_mask != 0) {
            fields[field_index++] = {current_mask, 0};
        }

        return fields;
    }

    template<typename... Args, std::size_t... Is>
    auto GetImplFunction(typename Dispatcher::ReturnType(Dispatcher::* impl)(Args...),
                         const std::array<FieldMask, sizeof...(Args)>& fields,
                         std::index_sequence<Is...>) {
        return [impl, fields](Dispatcher& dis, T opcode) -> typename Dispatcher::ReturnType {
            return (dis.*impl)(static_cast<Args>((opcode & fields[Is].mask) >> fields[Is].shift)...);
        };
    }
};

template<typename Dispatcher>
std::vector<Instruction<Thumb, Dispatcher>> GetThumbInstructionTable();
template<typename Dispatcher>
std::vector<Instruction<Arm, Dispatcher>> GetArmInstructionTable();

} // End namespace Gba
