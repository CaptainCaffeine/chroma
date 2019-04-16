// This file is a part of Chroma.
// Copyright (C) 2016-2019 Matthew Murray
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

#include <cstring>
#include <png.h>
#include <fmt/format.h>

#include "common/Screenshot.h"

namespace Common {

void WriteImageToFile(const std::vector<u8>& buffer, const std::string& filename, int width, int height) {
    png_image image;
    std::memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    image.format = PNG_FORMAT_RGB;
    image.width = width;
    image.height = height;
    image.flags = 0;
    image.colormap_entries = 0;

    png_image_write_to_file(&image, fmt::format("{}.png", filename).c_str(), false, buffer.data(), width * 3, nullptr);

    if (PNG_IMAGE_FAILED(image)) {
        fmt::print("Failed to write PNG image: {}", image.message);
    } else if (image.warning_or_error != 0) {
        fmt::print("Warning when writing PNG image: {}", image.message);
    }
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
