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
#include <array>

#include "emu/SDLContext.h"

namespace Emu {

SDLContext::SDLContext(unsigned int scale, bool fullscreen) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        throw std::runtime_error(GetSDLErrorString("Init"));
    }

    window = SDL_CreateWindow("Chroma",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              160 * scale,
                              144 * scale,
                              SDL_WINDOW_SHOWN);
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

    SDL_RenderSetLogicalSize(renderer, 160, 144);
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ABGR1555,
                                SDL_TEXTUREACCESS_STREAMING,
                                160,
                                144);
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
    want.format = AUDIO_U8;
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

void SDLContext::RenderFrame(const u16* fb_ptr) {
    SDL_LockTexture(texture, nullptr, &texture_pixels, &texture_pitch);
    memcpy(texture_pixels, fb_ptr, 160 * 144 * sizeof(u16));
    SDL_UnlockTexture(texture);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void SDLContext::ToggleFullscreen() {
    u32 fullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(window, fullscreen ^ SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void SDLContext::PushBackAudio(const std::vector<u8>& sample_buffer) {
    std::size_t remaining_buffer = 3200 - SDL_GetQueuedAudioSize(audio_device);
    std::size_t bytes_to_queue = std::min(remaining_buffer, sample_buffer.size());

    SDL_QueueAudio(audio_device, sample_buffer.data(), bytes_to_queue);
}

void SDLContext::UnpauseAudio() {
    SDL_PauseAudioDevice(audio_device, 0);
}

void SDLContext::PauseAudio() {
    SDL_PauseAudioDevice(audio_device, 1);
}

} // End namespace Emu
