#pragma once

#include <cassert>
#include <cstdint>
#include <memory>

class Cpu;
class Ppu;

class Cart {
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

  enum Mirroring { MIRROR_VERT, MIRROR_HORZ };

  class Header {
  public:
    Header(uint8_t bytes[16]);

    bool      has_trainer() const;
    bool      prg_ram_persistent() const;
    int       prg_rom_chunks() const;
    int       chr_rom_chunks() const;
    int       prg_ram_chunks() const;
    bool      chr_rom_readonly() const;
    Mirroring mirroring() const;
    bool      mirroring_specified() const;
    int       mapper() const;

  private:
    uint8_t bytes_[16];
  };

  struct Memory {
    std::unique_ptr<uint8_t[]> prg_rom;
    std::unique_ptr<uint8_t[]> chr_rom;
    std::unique_ptr<uint8_t[]> prg_ram;
    int                        prg_rom_size;
    int                        chr_rom_size;
    int                        prg_ram_size;
    bool                       chr_rom_readonly;
  };

  virtual ~Cart() = default;

  virtual void set_cpu(Cpu *) {}
  virtual void set_ppu(Ppu *) {}
  virtual void power_up() {}
  virtual void reset() {}

  virtual uint8_t peek_cpu(uint16_t addr)            = 0;
  virtual void    poke_cpu(uint16_t addr, uint8_t x) = 0;

  virtual PeekPpu peek_ppu(uint16_t addr)            = 0;
  virtual PokePpu poke_ppu(uint16_t addr, uint8_t x) = 0;

  // Returning false here means stop calling me on every PPU step (saves the
  // overhead of a virtual function call per step).
  virtual bool step_ppu() { return false; }

protected:
  static uint16_t mirrored_nt_addr(Mirroring mirroring, uint16_t addr);
};

std::unique_ptr<Cart> read_cart(std::ifstream &is);
