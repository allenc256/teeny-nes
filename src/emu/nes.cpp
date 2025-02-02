#include <fstream>

#include "src/emu/nes.h"

Nes::Nes() : powered_on_(false) {
  cpu_.set_apu(&apu_);
  cpu_.set_ppu(&ppu_);
  cpu_.set_input(&input_);
  cpu_.set_cart(&cart_);
  ppu_.set_cpu(&cpu_);
  ppu_.set_cart(&cart_);
  apu_.set_cpu(&cpu_);
  cart_.set_cpu(&cpu_);
  cart_.set_ppu(&ppu_);
}

void Nes::power_on() {
  if (powered_on_) {
    return;
  }
  if (!cart_.loaded()) {
    throw std::runtime_error("cannot power up without loading cart first");
  }

  cart_.power_on();
  cpu_.power_on();
  ppu_.power_on();
  apu_.power_on();
  input_.power_on();
  powered_on_ = true;
}

void Nes::power_off() {
  if (!powered_on_) {
    return;
  }
  cart_.power_off();
  powered_on_ = false;
}

void Nes::reset() {
  if (!powered_on_) {
    throw std::runtime_error("system hasn't been powered up yet");
  }

  cart_.reset();
  cpu_.reset();
  ppu_.reset();
  apu_.reset();
  input_.power_on();
}

void Nes::load_cart(const std::filesystem::path &path) {
  if (powered_on_) {
    throw std::runtime_error("cannot load cart when system is powered up");
  }

  cart_.load_cart(path);
}

void Nes::step() {
  cpu_.step();
  int ppu_catchup = (int)(cpu_to_ppu_cycles(cpu_.cycles()) - ppu_.cycles());
  for (int i = 0; i < ppu_catchup; i++) {
    ppu_.step();
  }
  int apu_catchup = (int)(cpu_.cycles() - apu_.cycles());
  for (int i = 0; i < apu_catchup; i++) {
    apu_.step();
  }
}
