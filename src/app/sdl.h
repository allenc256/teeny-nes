#pragma once

#include <SDL_audio.h>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class SDLRes {
public:
  SDLRes();
  ~SDLRes();

  float scale_factor() const { return scale_factor_; }

private:
  float scale_factor_;
};

class SDLWindowRes {
public:
  SDLWindowRes(const char *title, int width, int height);
  ~SDLWindowRes();
  SDL_Window *get() const { return window_; }

private:
  SDL_Window *window_;
};

class SDLRendererRes {
public:
  SDLRendererRes(SDL_Window *window);
  ~SDLRendererRes();

  SDL_Renderer *get() const { return renderer_; }

private:
  SDL_Renderer *renderer_;
};

class SDLTextureRes {
public:
  SDLTextureRes(SDL_Renderer *renderer, int width, int height);
  ~SDLTextureRes();

  SDL_Texture *get() const { return texture_; }

private:
  SDL_Texture *texture_;
};

void clear_texture(SDL_Texture *texture, int height);

class SDLAudioDeviceRes {
public:
  static constexpr int OUTPUT_RATE = 44100;

  SDLAudioDeviceRes();
  ~SDLAudioDeviceRes();

  SDL_AudioDeviceID get() const { return device_; }

private:
  SDL_AudioDeviceID device_;
};
