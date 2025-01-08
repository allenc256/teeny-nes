#pragma once

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>

class Cpu;
class Ppu;

enum Mirroring {
  MIRROR_VERT,
  MIRROR_HORZ,
  MIRROR_SCREEN_A_ONLY,
  MIRROR_SCREEN_B_ONLY
};

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

class CartHeader {
public:
  CartHeader(uint8_t bytes[16]);

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

struct CartMemory {
  std::unique_ptr<uint8_t[]> prg_rom;
  std::unique_ptr<uint8_t[]> chr_rom;
  std::unique_ptr<uint8_t[]> prg_ram;
  int                        prg_rom_size;
  int                        chr_rom_size;
  int                        prg_ram_size;
  bool                       chr_rom_readonly;
  bool                       prg_ram_persistent;
  std::filesystem::path      prg_ram_save_path;
};

class Mapper {
public:
  virtual ~Mapper() = default;

  virtual void power_on() {}
  virtual void reset() {}

  virtual uint8_t peek_cpu(uint16_t addr)            = 0;
  virtual void    poke_cpu(uint16_t addr, uint8_t x) = 0;

  virtual PeekPpu peek_ppu(uint16_t addr)            = 0;
  virtual PokePpu poke_ppu(uint16_t addr, uint8_t x) = 0;

  virtual bool step_ppu_enabled() { return false; }
  virtual void step_ppu() {}

protected:
  static uint16_t mirrored_nt_addr(Mirroring mirroring, uint16_t addr);
};
