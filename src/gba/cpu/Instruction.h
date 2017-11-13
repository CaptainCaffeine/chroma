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
#include <string>
#include <unordered_map>

#include <iostream>
#include <iomanip>

#include "common/CommonTypes.h"
#include "common/CommonFuncs.h"

namespace Gba {

using Arm = u32;
using Thumb = u16;

template<typename T>
class Instruction {
public:
    Instruction(const char* _disasm, const char* instr_layout)
            : disasm(_disasm) {

        CreateMasks(instr_layout);
    }

    bool Match(T opcode) const {
        return (opcode & fixed_mask) == instr_mask;
    }

    void GetFieldValues(T opcode) const {
        std::unordered_map<char, int> values;

        for (const auto& field : fields) {
            int field_value = (opcode & field.mask) >> field.shift;
            auto retval = values.emplace(field.name, field_value);

            if (!retval.second) {
                // If there's already a value for this field, it means this field's bits aren't consecutive,
                // i.e. "01000110dmmmmddd". So we shift the value of the first half by the number of bits in
                // the second half, and combine them.
                int second_half_length = Popcount(field.mask);
                int first_half = values[field.name];
                values[field.name] = (first_half << second_half_length) | field_value;
            }
        }

        //std::cout << disasm << "\n";
        //for (const auto& value : values) {
        //    std::cout << (value.first) << ": " << std::hex << std::uppercase << (value.second) << "\n";
        //}
        //std::cout << "\n";
    }
private:
    static constexpr auto num_bits = sizeof(T) * 8;

    const std::string disasm;

    T fixed_mask = 0;
    T instr_mask = 0;

    struct FieldMask {
        FieldMask(T _mask, int _shift, char _name) : mask(_mask), shift(_shift), name(_name) {}
        T mask;
        int shift;
        char name;
    };

    std::vector<FieldMask> fields;

    void CreateMasks(const std::string& instr_layout) {
        char last_bit = '0';
        T current_mask = 0;

        for (std::size_t i = 0; i < instr_layout.size(); ++i) {
            const char bit = instr_layout[i];
            const int shift = num_bits - 1 - i;
            const T bit_mask = 1 << shift;

            if (bit != last_bit && last_bit != '0' && last_bit != '1') {
                fields.emplace_back(current_mask, shift + 1, last_bit);
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

        fields.emplace_back(current_mask, 0, last_bit);
    }
};

} // End namespace Gba
