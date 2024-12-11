#pragma once

#include "src/app/sdl.h"
#include "src/emu/nes.h"

class PpuWindow {
public:
  PpuWindow(Nes &nes, SDL_Renderer *renderer);

  void render();

private:
  void render_pattern_table();
  void render_name_table();
  void render_palette();
  void render_stats();

  void prepare_pt_tex();
  void prepare_nt_tex();

  Nes          &nes_;
  SDLTextureRes pt_tex_;
  SDLTextureRes nt_tex_;
};
