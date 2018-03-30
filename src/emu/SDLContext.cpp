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

#include <stdexcept>
#include <fmt/format.h>

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
        SDL_ShowCursor(SDL_DISABLE);
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
    if (FullscreenEnabled()) {
        // We disable fullscreen to prevent the mouse from being moved on shutdown.
        ToggleFullscreen();
    }

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
    // SDL moves the mouse around when transitioning in and out of fullscreen, so we record the mouse position before
    // the transition and restore it afterwards.
    int x, y;
    SDL_GetGlobalMouseState(&x, &y);

    if (FullscreenEnabled()) {
        SDL_SetWindowFullscreen(window, 0);
        SDL_ShowCursor(SDL_ENABLE);
    } else {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowCursor(SDL_DISABLE);
    }

    SDL_WarpMouseGlobal(x, y);
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

void SDLContext::RegisterCallback(InputEvent event, std::function<void(bool)> callback) {
    input_callbacks.insert({event, callback});
}

void SDLContext::UpdateFrameTimes(float avg_time_us, float max_time_us) {
    SDL_SetWindowTitle(window, fmt::format("Chroma - avg {:0>4.1f}ms - max {:0>4.1f}ms",
                                           avg_time_us / 1000, max_time_us / 1000).data());
}

void SDLContext::PollEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            input_callbacks[InputEvent::Quit](true);
        } else if (e.type == SDL_KEYDOWN) {
            if (e.key.repeat != 0) {
                continue;
            }

            switch (e.key.keysym.sym) {
            case SDLK_q:
                if (SDL_GetModState() & KMOD_CTRL) {
                    input_callbacks[InputEvent::Quit](true);
                }
                break;
            case SDLK_p:
                input_callbacks[InputEvent::Pause](true);
                break;
            case SDLK_b:
                input_callbacks[InputEvent::LogLevel](true);
                break;
            case SDLK_v:
                input_callbacks[InputEvent::Fullscreen](true);
                break;
            case SDLK_t:
                input_callbacks[InputEvent::Screenshot](true);
                break;
            case SDLK_y:
                input_callbacks[InputEvent::LcdDebug](true);
                break;

            case SDLK_w:
                input_callbacks[InputEvent::Up](true);
                break;
            case SDLK_a:
                input_callbacks[InputEvent::Left](true);
                break;
            case SDLK_s:
                input_callbacks[InputEvent::Down](true);
                break;
            case SDLK_d:
                input_callbacks[InputEvent::Right](true);
                break;

            case SDLK_k:
                input_callbacks[InputEvent::A](true);
                break;
            case SDLK_j:
                input_callbacks[InputEvent::B](true);
                break;
            case SDLK_h:
                input_callbacks[InputEvent::L](true);
                break;
            case SDLK_l:
                input_callbacks[InputEvent::R](true);
                break;

            case SDLK_RETURN:
            case SDLK_i:
                input_callbacks[InputEvent::Start](true);
                break;
            case SDLK_u:
                input_callbacks[InputEvent::Select](true);
                break;
            default:
                break;
            }
        } else if (e.type == SDL_KEYUP) {
            if (e.key.repeat != 0) {
                continue;
            }

            switch (e.key.keysym.sym) {
            case SDLK_w:
                input_callbacks[InputEvent::Up](false);
                break;
            case SDLK_a:
                input_callbacks[InputEvent::Left](false);
                break;
            case SDLK_s:
                input_callbacks[InputEvent::Down](false);
                break;
            case SDLK_d:
                input_callbacks[InputEvent::Right](false);
                break;

            case SDLK_k:
                input_callbacks[InputEvent::A](false);
                break;
            case SDLK_j:
                input_callbacks[InputEvent::B](false);
                break;
            case SDLK_h:
                input_callbacks[InputEvent::L](false);
                break;
            case SDLK_l:
                input_callbacks[InputEvent::R](false);
                break;

            case SDLK_RETURN:
            case SDLK_i:
                input_callbacks[InputEvent::Start](false);
                break;
            case SDLK_u:
                input_callbacks[InputEvent::Select](false);
                break;
            default:
                break;
            }
        } else if (e.type == SDL_WINDOWEVENT) {
            switch (e.window.event) {
            case SDL_WINDOWEVENT_HIDDEN:
                input_callbacks[InputEvent::HideWindow](true);
                break;
            case SDL_WINDOWEVENT_SHOWN:
                input_callbacks[InputEvent::ShowWindow](true);
                break;
            default:
                break;
            }
        }
    }
}

} // End namespace Emu
