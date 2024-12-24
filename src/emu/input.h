#pragma once

#include <cstdint>

class Ppu;

class Controller {
public:
  enum ButtonFlags {
    BUTTON_A       = 1 << 0,
    BUTTON_B       = 1 << 1,
    BUTTON_SELECT  = 1 << 2,
    BUTTON_START   = 1 << 3,
    BUTTON_UP      = 1 << 4,
    BUTTON_DOWN    = 1 << 5,
    BUTTON_LEFT    = 1 << 6,
    BUTTON_RIGHT   = 1 << 7,
    BUTTON_TURBO_A = 1 << 8,
    BUTTON_TURBO_B = 1 << 9,
  };

  virtual ~Controller() = default;
  virtual int poll()    = 0;
};

class Input {
public:
  Input();

  void set_controller(Controller *controller, int index);

  void power_up();
  void reset();

  void    write_controller(uint8_t x);
  uint8_t read_controller(int index);

private:
  Controller *controllers_[2];
  uint8_t     shift_reg_[2];
  int         turbo_counter_;
  bool        strobe_;
};
