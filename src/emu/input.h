#pragma once

#include <cstdint>

class Ppu;

class Controller {
public:
  enum ButtonFlags {
    A      = 0b00000001,
    B      = 0b00000010,
    SELECT = 0b00000100,
    START  = 0b00001000,
    UP     = 0b00010000,
    DOWN   = 0b00100000,
    LEFT   = 0b01000000,
    RIGHT  = 0b10000000,
  };

  virtual ~Controller()                = default;
  virtual uint8_t poll(int64_t frames) = 0;
};

class Input {
public:
  Input();

  void set_ppu(Ppu *ppu);
  void set_controller(Controller *controller, int index);

  void power_up();
  void reset();

  void    write_controller(uint8_t x);
  uint8_t read_controller(int index);

private:
  Ppu        *ppu_;
  Controller *controllers_[2];
  uint8_t     shift_reg_[2];
  bool        strobe_;
};
