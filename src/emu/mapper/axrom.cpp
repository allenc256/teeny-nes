#include "src/emu/mapper/axrom.h"

AxRom::AxRom(CartMemory &mem)
    : mem_(mem),
      bank_addr_(0),
      mirroring_(MIRROR_SCREEN_A_ONLY) {}

uint8_t AxRom::peek_cpu(uint16_t addr) {
  if (addr >= 0x8000) {
    return mem_.prg_rom[bank_addr_ + addr - 0x8000];
  } else if (addr >= 0x6000) {
    return mem_.prg_ram[addr - 0x6000];
  } else {
    return 0;
  }
}

void AxRom::poke_cpu(uint16_t addr, uint8_t x) {
  if (addr >= 0x8000) {
    bank_addr_ = (x & 7) << 15;
    mirroring_ = x & 0x10 ? MIRROR_SCREEN_B_ONLY : MIRROR_SCREEN_A_ONLY;
  } else if (addr >= 0x6000) {
    mem_.prg_ram[addr - 0x6000] = x;
  } else {
    // no-op
  }
}

PeekPpu AxRom::peek_ppu(uint16_t addr) {
  if (addr >= 0x3000) {
    return PeekPpu::make_value(0);
  } else if (addr >= 0x2000) {
    return PeekPpu::make_address(mirrored_nt_addr(mirroring_, addr));
  } else {
    return PeekPpu::make_value(mem_.chr_rom[addr]);
  }
}

PokePpu AxRom::poke_ppu(uint16_t addr, uint8_t x) {
  if (addr >= 0x3000) {
    return PokePpu::make_success(); // no-op
  } else if (addr >= 0x2000) {
    return PokePpu::make_address(mirrored_nt_addr(mirroring_, addr));
  } else {
    if (!mem_.chr_rom_readonly) {
      mem_.chr_rom[addr] = x;
    }
    return PokePpu::make_success();
  }
}
