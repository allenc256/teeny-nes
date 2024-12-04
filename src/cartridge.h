#pragma once

#include <cstdint>
#include <memory>

class Cartridge {
public:
  static constexpr uint16_t ADDR_START = 0x4020;

  virtual ~Cartridge() = default;

  virtual uint8_t peek(uint16_t address) const          = 0;
  virtual uint8_t poke(uint16_t address, uint8_t value) = 0;
};

std::unique_ptr<Cartridge> read_cartridge(std::ifstream &is);
