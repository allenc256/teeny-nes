#pragma once

#include "src/app/sdl.h"
#include "src/emu/nes.h"

class GameWindow {
public:
  static constexpr float SCALE_X       = 3.0f;
  static constexpr float SCALE_Y       = 2.25f;
  static constexpr int   OVERSCAN      = 8;
  static constexpr int   FRAME_WIDTH   = 256;
  static constexpr int   FRAME_HEIGHT  = 240 - OVERSCAN * 2;
  static constexpr int   WINDOW_WIDTH  = (int)(FRAME_WIDTH * SCALE_X);
  static constexpr int   WINDOW_HEIGHT = (int)(FRAME_HEIGHT * SCALE_Y);

  GameWindow(Nes &nes, SDL_Renderer *renderer);

  void render(bool show_tooltip);

private:
  void prepare_frame();

  Nes          &nes_;
  SDLTextureRes frame_;
};
