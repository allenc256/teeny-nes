#include <SDL.h>
#include <cstring>
#include <format>
#include <stdexcept>

#include "sdl.h"

SDLRes::SDLRes() {
  if (SDL_Init(
          SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER |
          SDL_INIT_GAMECONTROLLER
      ) != 0) {
    throw std::runtime_error(
        std::format("failed to initialize SDL: {}", SDL_GetError())
    );
  }

#ifdef __APPLE__
  scale_factor_ = 1.0f;
#else
  constexpr float default_dpi = 96.0f;
  float           dpi;
  if (SDL_GetDisplayDPI(0, nullptr, &dpi, nullptr)) {
    dpi = default_dpi;
  }
  scale_factor_ = dpi / default_dpi;
#endif
}

SDLRes::~SDLRes() { SDL_Quit(); }

SDLWindowRes::SDLWindowRes(const char *title, int width, int height) {
  SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  window_ = SDL_CreateWindow(
      title,
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width,
      height,
      window_flags
  );
  if (!window_) {
    throw std::runtime_error(
        std::format("failed to create window: {}", SDL_GetError())
    );
  }
}

SDLWindowRes::~SDLWindowRes() { SDL_DestroyWindow(window_); }

SDLRendererRes::SDLRendererRes(SDL_Window *window) {
  renderer_ = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
  );
  if (!renderer_) {
    throw std::runtime_error(
        std::format("failed to create renderer: {}", SDL_GetError())
    );
  }
}

SDLRendererRes::~SDLRendererRes() { SDL_DestroyRenderer(renderer_); }

SDLTextureRes::SDLTextureRes(SDL_Renderer *renderer, int width, int height) {
  texture_ = SDL_CreateTexture(
      renderer,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STREAMING,
      width,
      height
  );
  if (!texture_) {
    throw std::runtime_error(
        std::format("failed to create texture: {}", SDL_GetError())
    );
  }
}

SDLTextureRes::~SDLTextureRes() { SDL_DestroyTexture(texture_); }

void clear_texture(SDL_Texture *texture, int height) {
  int   pitch;
  void *pixels;
  SDL_LockTexture(texture, nullptr, &pixels, &pitch);
  std::memset(pixels, 0, pitch * height);
  SDL_UnlockTexture(texture);
}

SDLAudioDeviceRes::SDLAudioDeviceRes() {
  SDL_AudioSpec audio_spec;
  SDL_zero(audio_spec);
  audio_spec.freq     = 44100;
  audio_spec.format   = AUDIO_F32SYS;
  audio_spec.channels = 1;
  audio_spec.samples  = 1024;
  audio_spec.callback = NULL;

  device_ = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);
  if (!device_) {
    throw std::runtime_error(
        std::format("failed to open audio device: {}", SDL_GetError())
    );
  }
}

SDLAudioDeviceRes::~SDLAudioDeviceRes() { SDL_CloseAudioDevice(device_); }
