#pragma once

#include "apu.h"
#include "cartridge.h"

class BusBase {
public:
  virtual ~BusBase() = default;

  virtual uint8_t peek(uint16_t addr)            = 0;
  virtual void    poke(uint16_t addr, uint8_t x) = 0;
};

class Bus : public BusBase {
public:
  Bus(Cartridge &cart) : ram_{0}, cart_(cart) {}

  uint8_t peek(uint16_t addr) override;
  void    poke(uint16_t addr, uint8_t x) override;

private:
  static constexpr int RAM_SIZE = 0x0800;
  static constexpr int RAM_MASK = 0x07ff;
  static constexpr int RAM_END  = 0x2000;

  uint8_t    ram_[RAM_SIZE];
  Cartridge &cart_;
  Apu        apu_;
};

class TestBus : public BusBase {
public:
  TestBus() : ram_{0} {}

  uint8_t peek(uint16_t addr) override;
  void    poke(uint16_t addr, uint8_t x) override;

private:
  uint8_t ram_[64 * 1024];
};
