#include <cassert>
#include <format>
#include <iostream>
#include <stdexcept>

#include "src/emu/input.h"

Input::Input() : controllers_{0}, turbo_counter_(0), strobe_(false) {}

void Input::power_up() { strobe_ = false; }
void Input::reset() { strobe_ = false; }

void Input::set_controller(Controller *controller, int index) {
  if (!(index >= 0 && index < 2)) {
    throw std::runtime_error(std::format("invalid controller index: {}", index)
    );
  }
  controllers_[index] = controller;
}

void Input::write_controller(uint8_t x) {
  using enum Controller::ButtonFlags;
  strobe_ = x & 1;
  if (strobe_) {
    for (int i = 0; i < 2; i++) {
      auto controller = controllers_[i];
      int  flags      = controller ? controller->poll() : 0;
      if (turbo_counter_ <= 3) {
        if (flags & BUTTON_TURBO_A) {
          flags |= BUTTON_A;
        }
        if (flags & BUTTON_TURBO_B) {
          flags |= BUTTON_B;
        }
      }
      shift_reg_[i] = (uint8_t)flags;
    }
    turbo_counter_ = (turbo_counter_ + 1) & 7;
  }
}

uint8_t Input::read_controller(int index) {
  assert(index >= 0 && index < 2);
  uint8_t result    = shift_reg_[index] & 1;
  shift_reg_[index] = (shift_reg_[index] >> 1) | 0x80;
  return result;
}
