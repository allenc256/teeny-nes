#include <cassert>
#include <format>
#include <iostream>
#include <stdexcept>

#include "src/emu/input.h"
#include "src/emu/ppu.h"

Input::Input() : controllers_{0}, strobe_(false) {}

void Input::power_up() { strobe_ = false; }
void Input::reset() { strobe_ = false; }
void Input::set_ppu(Ppu *ppu) { ppu_ = ppu; }

void Input::set_controller(Controller *controller, int index) {
  if (!(index >= 0 && index < 2)) {
    throw std::runtime_error(std::format("invalid controller index: {}", index)
    );
  }
  controllers_[index] = controller;
}

void Input::write_controller(uint8_t x) {
  strobe_ = x & 1;
  if (strobe_) {
    for (int i = 0; i < 2; i++) {
      auto controller = controllers_[i];
      shift_reg_[i]   = controller ? controller->poll(ppu_->frames()) : 0;
    }
  }
}

uint8_t Input::read_controller(int index) {
  assert(index >= 0 && index < 2);
  uint8_t result    = shift_reg_[index] & 1;
  shift_reg_[index] = (shift_reg_[index] >> 1) | 0x80;
  return result;
}
