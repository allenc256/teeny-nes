#pragma once

#include "src/emu/apu.h"
#include "src/emu/cart.h"
#include "src/emu/cpu.h"
#include "src/emu/input.h"
#include "src/emu/ppu.h"

class Nes {
public:
  Nes();

  Cpu   &cpu() { return cpu_; }
  Ppu   &ppu() { return ppu_; }
  Apu   &apu() { return apu_; }
  Input &input() { return input_; }
  Cart  &cart() { return cart_; }

  void power_on();
  void power_off();
  void reset();
  void step();
  bool is_powered_on() const { return powered_on_; }

  void load_cart(const std::string &path);

private:
  Cpu   cpu_;
  Ppu   ppu_;
  Apu   apu_;
  Input input_;
  Cart  cart_;
  bool  powered_on_;
};
