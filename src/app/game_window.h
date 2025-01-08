#pragma once

#include "src/app/sdl.h"
#include "src/emu/nes.h"

class GameWindow {
public:
  GameWindow(Nes &nes, SDL_Renderer *renderer);

  bool focused() const { return focused_; }
  void render();

private:
  void prepare_frame();

  Nes          &nes_;
  SDLTextureRes frame_;
  bool          focused_;
};
