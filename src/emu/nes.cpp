#include <fstream>

#include "src/emu/nes.h"

Nes::Nes() {
  cpu_.set_apu(&apu_);
  cpu_.set_ppu(&ppu_);
  ppu_.set_cpu(&cpu_);
}

void Nes::power_up() {
  if (!cart_) {
    throw std::runtime_error("cannot power up without cart");
  }

  cpu_.power_up();
  ppu_.power_up();
  apu_.power_up();
}

void Nes::reset() {
  cpu_.reset();
  ppu_.reset();
  apu_.reset();
}

void Nes::load_cart(std::string_view path) {
  std::ifstream is(path, std::ios::binary);
  cart_ = read_cart(is);

  cpu_.set_cart(cart_.get());
  ppu_.set_cart(cart_.get());
}

void Nes::step() {
  cpu_.step();

  int ppu_catchup = (int)(cpu_.cycles() * 3 - ppu_.cycles());
  for (int i = 0; i < ppu_catchup; i++) {
    ppu_.step();
  }
}
