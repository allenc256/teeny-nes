#pragma once

#include "src/emu/input.h"

class KeyboardController : public Controller {
public:
  uint8_t poll() override;
};
