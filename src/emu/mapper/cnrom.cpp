#include "src/emu/mapper/cnrom.h"

CnRom::CnRom(const CartHeader &header, CartMemory &mem)
    : mem_(mem),
      bank_addr_(0) {
  if (header.mirroring_specified()) {
    mirroring_ = header.mirroring();
  } else {
    throw std::runtime_error("unspecified mirroring");
  }
}

uint8_t CnRom::peek_cpu(uint16_t addr) {
  if (addr >= 0x8000) {
    return mem_.prg_rom[addr - 0x8000];
  } else if (addr >= 0x6000) {
    return mem_.prg_ram[addr - 0x6000];
  } else {
    return 0;
  }
}

void CnRom::poke_cpu(uint16_t addr, uint8_t x) {
  if (addr >= 0x8000) {
    bank_addr_ = (x & 3) << 13;
  } else if (addr >= 0x6000) {
    mem_.prg_ram[addr - 0x6000] = x;
  } else {
    // no-op
  }
}

PeekPpu CnRom::peek_ppu(uint16_t addr) {
  if (addr >= 0x3000) {
    return PeekPpu::make_value(0);
  } else if (addr >= 0x2000) {
    return PeekPpu::make_address(mirrored_nt_addr(mirroring_, addr));
  } else {
    return PeekPpu::make_value(mem_.chr_rom[bank_addr_ + addr]);
  }
}

PokePpu CnRom::poke_ppu(uint16_t addr, uint8_t x) {
  if (addr >= 0x3000) {
    return PokePpu::make_success(); // no-op
  } else if (addr >= 0x2000) {
    return PokePpu::make_address(mirrored_nt_addr(mirroring_, addr));
  } else {
    if (!mem_.chr_rom_readonly) {
      mem_.chr_rom[bank_addr_ + addr] = x;
    }
    return PokePpu::make_success();
  }
}
