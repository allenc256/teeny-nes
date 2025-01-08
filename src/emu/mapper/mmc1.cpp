#include <cstring>

#include "src/emu/bits.h"
#include "src/emu/mapper/mmc1.h"

static constexpr uint8_t SHIFT_REG_RESET_FLAG  = 0b10000000;
static constexpr uint8_t SHIFT_REG_RESET_VAL   = 0b00010000;
static constexpr uint8_t CONTROL_REG_RESET_VAL = 0b00001100;

static constexpr uint16_t PRG_RAM_START    = 0x6000;
static constexpr uint16_t PRG_BANK_0_START = 0x8000;
static constexpr uint16_t PRG_BANK_1_START = 0xc000;
static constexpr uint16_t CHR_BANK_0_START = 0x0000;
static constexpr uint16_t CHR_BANK_1_START = 0x1000;

Mmc1::Mmc1([[maybe_unused]] const Header &header, Memory &&mem)
    : Cart(std::move(mem)) {}

void Mmc1::internal_power_on() { internal_reset(); }

void Mmc1::internal_reset() {
  std::memset(&regs_, 0, sizeof(regs_));
  std::memset(mem_.prg_ram.get(), 0, mem_.prg_ram_size);
  regs_.shift   = SHIFT_REG_RESET_VAL;
  regs_.control = CONTROL_REG_RESET_VAL;
}

uint8_t Mmc1::peek_cpu(uint16_t addr) {
  if (addr >= PRG_BANK_0_START) {
    return mem_.prg_rom[map_prg_rom_addr(addr)];
  } else if (addr >= PRG_RAM_START) {
    return mem_.prg_ram[addr - PRG_RAM_START];
  } else {
    return 0;
  }
}

void Mmc1::poke_cpu(uint16_t addr, uint8_t x) {
  if (addr >= PRG_BANK_0_START) {
    write_shift_reg(addr, x);
  } else if (addr >= PRG_RAM_START) {
    mem_.prg_ram[addr - PRG_RAM_START] = x;
  } else {
    // no-op
  }
}

Mmc1::PeekPpu Mmc1::peek_ppu(uint16_t addr) {
  if (addr >= 0x3000) {
    return PeekPpu::make_value(0);
  } else if (addr >= 0x2000) {
    return PeekPpu::make_address(mirrored_nt_addr(mirroring(), addr));
  } else {
    return PeekPpu::make_value(mem_.chr_rom[map_chr_rom_addr(addr)]);
  }
}

Mmc1::PokePpu Mmc1::poke_ppu(uint16_t addr, uint8_t x) {
  if (addr >= 0x3000) {
    return PokePpu::make_success();
  } else if (addr >= 0x2000) {
    return PokePpu::make_address(mirrored_nt_addr(mirroring(), addr));
  } else {
    if (!mem_.chr_rom_readonly) {
      mem_.chr_rom[map_chr_rom_addr(addr)] = x;
    }
    return PokePpu::make_success();
  }
}

void Mmc1::write_shift_reg(uint16_t addr, uint8_t x) {
  if (x & SHIFT_REG_RESET_FLAG) {
    regs_.shift = SHIFT_REG_RESET_VAL;
    regs_.control |= CONTROL_REG_RESET_VAL;
    return;
  } else {
    bool full   = regs_.shift & 1;
    regs_.shift = (uint8_t)((regs_.shift >> 1) | ((x & 1) << 4));
    if (full) {
      switch (addr & 0xe000) {
      case 0x8000: regs_.control = regs_.shift; break;
      case 0xa000: regs_.chr_bank_0 = regs_.shift; break;
      case 0xc000: regs_.chr_bank_1 = regs_.shift; break;
      default: regs_.prg_bank = regs_.shift; break;
      }
      regs_.shift = SHIFT_REG_RESET_VAL;
    }
  }
}

int Mmc1::map_prg_rom_addr(uint16_t addr) const {
  assert(addr >= PRG_BANK_0_START);
  int bank, offset;
  switch ((regs_.control >> 2) & 3) {
  case 0:
  case 1:
    bank   = regs_.prg_bank & 0b1110;
    offset = addr - PRG_BANK_0_START;
    break;
  case 2:
    if (addr >= PRG_BANK_1_START) {
      bank   = regs_.prg_bank & 0b1111;
      offset = addr - PRG_BANK_1_START;
    } else {
      bank   = 0;
      offset = addr - PRG_BANK_0_START;
    }
    break;
  default:
    if (addr >= PRG_BANK_1_START) {
      bank   = prg_rom_banks() - 1;
      offset = addr - PRG_BANK_1_START;
    } else {
      bank   = regs_.prg_bank & 0b1111;
      offset = addr - PRG_BANK_0_START;
    }
    break;
  }
  return (bank << 14) + offset;
}

int Mmc1::map_chr_rom_addr(uint16_t addr) const {
  assert(addr <= 0x1fff);
  int bank, offset;
  if (!(regs_.control & 0b10000)) {
    bank   = regs_.chr_bank_0 & 0b11110;
    offset = addr;
  } else if (addr >= CHR_BANK_1_START) {
    bank   = regs_.chr_bank_1;
    offset = addr - CHR_BANK_1_START;
  } else {
    bank   = regs_.chr_bank_0;
    offset = addr - CHR_BANK_0_START;
  }
  return (bank << 12) + offset;
}

Cart::Mirroring Mmc1::mirroring() const {
  switch (regs_.control & 3) {
  case 0: return MIRROR_SCREEN_A_ONLY;
  case 1: return MIRROR_SCREEN_B_ONLY;
  case 2: return MIRROR_VERT;
  default: return MIRROR_HORZ;
  }
}

int Mmc1::prg_rom_banks() const { return mem_.prg_rom_size >> 14; }
