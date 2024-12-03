#pragma once

#include <cstdint>
#include <memory>

class Cartridge {
public:
  static constexpr uint16_t CART_OFFSET = 0x4020;

  virtual ~Cartridge() = default;

  virtual uint8_t peek(uint16_t address) const = 0;
};

std::unique_ptr<Cartridge> read_cartridge(std::ifstream &is);
