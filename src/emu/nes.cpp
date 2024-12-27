#include <fstream>

#include "src/emu/cycles.h"
#include "src/emu/nes.h"

Nes::Nes() : powered_on_(false) {
  cpu_.set_apu(&apu_);
  cpu_.set_ppu(&ppu_);
  cpu_.set_input(&input_);
  ppu_.set_cpu(&cpu_);
  apu_.set_cpu(&cpu_);
}

void Nes::power_on() {
  if (!cart_) {
    throw std::runtime_error("cannot power up without loading cart first");
  }
  if (powered_on_) {
    throw std::runtime_error("system already powered up");
  }

  cart_->power_on();
  cpu_.power_on();
  ppu_.power_on();
  apu_.power_on();
  input_.power_on();
  powered_on_ = true;
}

void Nes::power_off() { powered_on_ = false; }

void Nes::reset() {
  if (!powered_on_) {
    throw std::runtime_error("system hasn't been powered up yet");
  }

  cart_->reset();
  cpu_.reset();
  ppu_.reset();
  apu_.reset();
  input_.power_on();
}

void Nes::load_cart(std::string_view path) {
  if (powered_on_) {
    throw std::runtime_error("cannot load cart when system is powered up");
  }

  std::ifstream is(path, std::ios::binary);
  cart_ = read_cart(is);

  cpu_.set_cart(cart_.get());
  ppu_.set_cart(cart_.get());
  cart_->set_cpu(&cpu_);
  cart_->set_ppu(&ppu_);
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

void Nes::step(int cpu_cycles) {
  int64_t target = cpu_.cycles() + cpu_cycles;
  while (cpu_.cycles() < target) {
    step();
  }
}
