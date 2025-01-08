#pragma once

#include "src/emu/input.h"

class KeyboardController : public Controller {
public:
  void set_enabled(bool enabled) { enabled_ = enabled; }

  int poll() override;

private:
  bool enabled_ = true;
};
