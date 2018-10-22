// This file is a part of Chroma.
// Copyright (C) 2016-2018 Matthew Murray
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

#include <string>
#include <array>
#include <unordered_map>
#include <functional>
#include <SDL.h>

#include "common/CommonTypes.h"

namespace Emu {

enum class InputEvent {Quit,
                       Pause,
                       LogLevel,
                       Fullscreen,
                       Screenshot,
                       LcdDebug,
                       HideWindow,
                       ShowWindow,
                       FrameAdvance,
                       Up,
                       Left,
                       Down,
                       Right,
                       A,
                       B,
                       L,
                       R,
                       Start,
                       Select};

class SDLContext {
public:
    SDLContext(int _width, int _height, unsigned int scale, bool fullscreen);
    ~SDLContext();

    void RenderFrame(const u16* fb_ptr) noexcept;
    void ToggleFullscreen() noexcept;

    void PushBackAudio(const std::array<s16, 1600>& sample_buffer) noexcept;
    void UnpauseAudio() noexcept;
    void PauseAudio() noexcept;

    void RegisterCallback(InputEvent event, std::function<void(bool)> callback);
    void PollEvents();

    void UpdateFrameTimes(float avg_frame_time, float max_frame_time);

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_AudioDeviceID audio_device;

    const int width;
    const int height;

    int texture_pitch;
    void* texture_pixels;

    std::unordered_map<InputEvent, std::function<void(bool)>> input_callbacks;

    bool FullscreenEnabled() const noexcept { return SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP; }
    static const std::string GetSDLErrorString(const std::string& error_function) {
        return {"SDL_" + error_function + " Error: " + SDL_GetError()};
    }
};

} // End namespace Emu
