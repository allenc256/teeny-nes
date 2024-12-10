#pragma once

#include "src/app/pixel.h"
#include "src/app/sdl.h"
#include "src/emu/nes.h"

class PatternTable {
public:
  PatternTable(Nes &nes, SDL_Renderer *renderer);

  void render();

private:
  void prepare_textures();

  Nes          &nes_;
  SDLTextureRes textures_[2];
  float         scale_;
};

void extract_pattern(
    Ppu        &ppu,
    uint16_t    pattern_base_addr,
    int         pattern,
    Pixel      *pix,
    int         pix_pitch,
    int         pix_x,
    int         pix_y,
    const Pixel palette[4]
);
