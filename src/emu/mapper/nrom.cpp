#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "src/emu/mapper/nrom.h"

static constexpr uint16_t PRG_ROM_MASK_128  = 0b0011111111111111;
static constexpr uint16_t PRG_ROM_MASK_256  = 0b0111111111111111;
static constexpr uint16_t PATTERN_TABLE_END = 0x2000;
static constexpr uint16_t NAME_TABLE_END    = 0x3000;

NRom::NRom(const Header &header, Memory &&mem)
    : Cart(std::move(mem)),
      mirroring_(header.mirroring()) {
  if (header.prg_rom_chunks() == 1) {
    prg_rom_mask_ = PRG_ROM_MASK_128;
  } else if (header.prg_rom_chunks() == 2) {
    prg_rom_mask_ = PRG_ROM_MASK_256;
  } else {
    throw std::runtime_error(
        std::format("bad PRG ROM size: {}", header.prg_rom_chunks())
    );
  }
}

uint8_t NRom::peek_cpu(uint16_t addr) {
  assert(addr >= CPU_ADDR_START);
  if (addr >= 0x8000) {
    return mem_.prg_rom[addr & prg_rom_mask_];
  } else if (addr >= 0x6000) {
    return mem_.prg_ram[addr - 0x6000];
  } else {
    return 0;
  }
}

void NRom::poke_cpu(uint16_t addr, uint8_t x) {
  if (addr >= 0x6000 && addr <= 0x7fff) {
    mem_.prg_ram[addr - 0x6000] = x;
  } else {
    // no-op
  }
}

NRom::PeekPpu NRom::peek_ppu(uint16_t addr) {
  assert(addr < 0x3f00);
  if (addr < PATTERN_TABLE_END) {
    return PeekPpu::make_value(mem_.chr_rom[addr]);
  } else if (addr < NAME_TABLE_END) {
    return PeekPpu::make_address(mirrored_nt_addr(mirroring_, addr));
  } else {
    return PeekPpu::make_value(0); // unused
  }
}

NRom::PokePpu NRom::poke_ppu(uint16_t addr, uint8_t x) {
  assert(addr < 0x3f00);
  if (addr < PATTERN_TABLE_END) {
    if (mem_.chr_rom_readonly) {
      // N.B., some games (e.g., 1942) explicitly contain writes to the CHR ROM
      // region as a form of copy-protection. These should be treated as no-ops.
    } else {
      mem_.chr_rom[addr] = x;
    }
    return PokePpu::make_success();
  } else if (addr < NAME_TABLE_END) {
    return PokePpu::make_address(mirrored_nt_addr(mirroring_, addr));
  } else {
    return PokePpu::make_success(); // no-op
  }
}
