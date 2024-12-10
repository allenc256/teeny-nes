#pragma once

struct SDL_Window;
struct SDL_Renderer;

class ImguiRes {
public:
  ImguiRes(SDL_Window *window, SDL_Renderer *renderer);
  ~ImguiRes();
};
