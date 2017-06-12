// This file is a part of Chroma.
// Copyright (C) 2016-2017 Matthew Murray
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

#include <fstream>

#include "common/Util.h"

namespace Common {

void WritePPMFile(const std::vector<u8>& buffer, const std::string& filename, int width, int height) {
    std::ofstream output_image(filename);

    output_image << "P6\n";
    output_image << width << " " << height << "\n";
    output_image << 255 << "\n";
    output_image.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * 3);
}

std::vector<u8> BGR5ToRGB8(const std::vector<u16>& bgr5_buffer) {
    std::vector<u8> rgb8_buffer;
    for (const u16 c : bgr5_buffer) {
        u8 blue = (c & 0x7C00) >> 10;
        blue = (blue * 255) / 31;
        u8 green = (c & 0x03E0) >> 5;
        green = (green * 255) / 31;
        u8 red = (c & 0x001F);
        red = (red * 255) / 31;

        rgb8_buffer.push_back(red);
        rgb8_buffer.push_back(green);
        rgb8_buffer.push_back(blue);
    }

    return rgb8_buffer;
}

} // End namespace Common
