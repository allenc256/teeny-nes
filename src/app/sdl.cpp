#include <SDL.h>
#include <format>
#include <stdexcept>

#include "sdl.h"

SDLRes::SDLRes() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) !=
      0) {
    throw std::runtime_error(
        std::format("failed to initialize SDL: {}", SDL_GetError())
    );
  }
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
