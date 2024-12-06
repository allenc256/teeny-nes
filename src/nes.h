#pragma once

#include "apu.h"
#include "cartridge.h"
#include "cpu.h"
#include "ppu.h"

class Nes {
public:
  Nes();

  void power_up();
  void reset();

  void load_cart(std::string_view path);

private:
  Cpu                        cpu_;
  Ppu                        ppu_;
  Apu                        apu_;
  std::unique_ptr<Cartridge> cart_;
};
