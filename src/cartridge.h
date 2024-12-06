#pragma once

#include <cassert>
#include <cstdint>
#include <memory>

class Cartridge {
public:
  static constexpr uint16_t PPU_ADDR_END   = 0x3f00;
  static constexpr uint16_t CPU_ADDR_START = 0x4020;

  class PeekPpu {
  public:
    bool is_address() const { return bits_ & ADDR_FLAG; }
    bool is_value() const { return !is_address(); }

    uint16_t address() const {
      assert(is_address());
      return bits_ & ADDR_MASK;
    }

    uint8_t value() const {
      assert(is_value());
      return (uint8_t)bits_;
    }

    static PeekPpu make_address(uint16_t addr) {
      assert(addr == (addr & ADDR_MASK));
      return PeekPpu(addr | ADDR_FLAG);
    }

    static PeekPpu make_value(uint8_t x) { return PeekPpu(x); }

  private:
    static constexpr uint16_t ADDR_FLAG = 0x8000;
    static constexpr uint16_t ADDR_MASK = 0x07ff;

    PeekPpu(uint16_t bits) : bits_(bits) {}

    uint16_t bits_;
  };

  class PokePpu {
  public:
    bool is_address() const { return bits_ & ADDR_FLAG; }

    uint16_t address() const {
      assert(is_address());
      return bits_ & ADDR_MASK;
    }

    static PokePpu make_address(uint16_t addr) {
      assert(!(addr & ~ADDR_MASK));
      return PokePpu(addr | ADDR_FLAG);
    }

    static PokePpu make_success() { return PokePpu(0); }

  private:
    static constexpr uint16_t ADDR_FLAG = 0x8000;
    static constexpr uint16_t ADDR_MASK = 0x07ff;

    PokePpu(uint16_t bits) : bits_(bits) {}

    uint16_t bits_;
  };

  virtual ~Cartridge() = default;

  virtual uint8_t peek_cpu(uint16_t addr)            = 0;
  virtual void    poke_cpu(uint16_t addr, uint8_t x) = 0;

  virtual PeekPpu peek_ppu(uint16_t addr)            = 0;
  virtual PokePpu poke_ppu(uint16_t addr, uint8_t x) = 0;
};

std::unique_ptr<Cartridge> read_cartridge(std::ifstream &is);
