#pragma once

#include "src/emu/input.h"

class KeyboardController : public Controller {
public:
  int poll() override;
};
