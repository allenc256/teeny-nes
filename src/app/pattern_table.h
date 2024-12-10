#pragma once

#include "src/app/sdl.h"

class PatternTable {
public:
  PatternTable(SDL_Renderer *renderer);

  void render();

private:
  void prepare_textures();

  SDLTextureRes textures_[2];
};
