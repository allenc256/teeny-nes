#pragma once

#include "apu.h"
#include "cart.h"
#include "cpu.h"
#include "ppu.h"

class Nes {
public:
  Nes();

  Cpu  &cpu() { return cpu_; }
  Ppu  &ppu() { return ppu_; }
  Apu  &apu() { return apu_; }
  Cart *cart() { return cart_.get(); }

  void power_up();
  void reset();

  void load_cart(std::string_view path);

private:
  Cpu                   cpu_;
  Ppu                   ppu_;
  Apu                   apu_;
  std::unique_ptr<Cart> cart_;
};
