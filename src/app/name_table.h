#pragma once

#include "src/app/sdl.h"

struct SDL_Renderer;
class Nes;

class NameTable {
public:
  NameTable(Nes &nes, SDL_Renderer *renderer);

  void render();

private:
  void prepare_texture();

  Nes          &nes_;
  SDLTextureRes texture_;
};
