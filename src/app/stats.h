#pragma once

#include "src/emu/nes.h"

class Stats {
public:
  Stats(Nes &nes);

  void render();

private:
  Nes &nes_;
};
