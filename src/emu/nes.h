#pragma once

#include "src/emu/apu.h"
#include "src/emu/cart.h"
#include "src/emu/cpu.h"
#include "src/emu/ppu.h"

class Nes {
public:
  Nes();

  Cpu  &cpu() { return cpu_; }
  Ppu  &ppu() { return ppu_; }
  Apu  &apu() { return apu_; }
  Cart *cart() { return cart_.get(); }

  void power_up();
  void reset();
  void step();

  void load_cart(std::string_view path);

private:
  Cpu                   cpu_;
  Ppu                   ppu_;
  Apu                   apu_;
  std::unique_ptr<Cart> cart_;
};
