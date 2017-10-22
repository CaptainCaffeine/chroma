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

#include <stdexcept>

#include "emu/SDLContext.h"

namespace Emu {

SDLContext::SDLContext(int _width, int _height, unsigned int scale, bool fullscreen)
        : width(_width)
        , height(_height) {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        throw std::runtime_error(GetSDLErrorString("Init"));
    }

    window = SDL_CreateWindow("Chroma",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              width * scale,
                              height * scale,
                              SDL_WINDOW_OPENGL);
    if (window == nullptr) {
        SDL_Quit();
        throw std::runtime_error(GetSDLErrorString("CreateWindow"));
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error(GetSDLErrorString("CreateRenderer"));
    }

    SDL_RenderSetLogicalSize(renderer, width, height);
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ABGR1555,
                                SDL_TEXTUREACCESS_STREAMING,
                                width,
                                height);
    if (texture == nullptr) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error(GetSDLErrorString("CreateTexture"));
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);

    if (fullscreen) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }

    SDL_AudioSpec want, have;
    want.freq = 48000;
    want.format = AUDIO_S16;
    want.channels = 2;
    want.samples = 1600;
    want.callback = nullptr; // nullptr for using a queue instead.

    audio_device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);

    if (audio_device == 0) {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error(GetSDLErrorString("OpenAudioDevice"));
    }
}

SDLContext::~SDLContext() {
    SDL_CloseAudioDevice(audio_device);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void SDLContext::RenderFrame(const u16* fb_ptr) noexcept {
    SDL_LockTexture(texture, nullptr, &texture_pixels, &texture_pitch);
    memcpy(texture_pixels, fb_ptr, width * height * sizeof(u16));
    SDL_UnlockTexture(texture);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void SDLContext::ToggleFullscreen() noexcept {
    u32 fullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(window, fullscreen ^ SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void SDLContext::PushBackAudio(const std::array<s16, 1600>& sample_buffer) noexcept {
    SDL_QueueAudio(audio_device, sample_buffer.data(), sample_buffer.size() * sizeof(s16));
}

void SDLContext::UnpauseAudio() noexcept {
    SDL_PauseAudioDevice(audio_device, 0);
}

void SDLContext::PauseAudio() noexcept {
    SDL_PauseAudioDevice(audio_device, 1);
}

} // End namespace Emu
