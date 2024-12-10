#pragma once

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class SDLRes {
public:
  SDLRes();
  ~SDLRes();
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
