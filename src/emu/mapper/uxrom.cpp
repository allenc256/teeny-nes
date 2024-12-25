#include <cstring>
#include <format>
#include <fstream>
#include <stdexcept>

#include "src/emu/mapper/uxrom.h"

UxRom::UxRom(const Header &header, Memory &&mem)
    : mem_(std::move(mem)),
      mirroring_(header.mirroring()),
      curr_bank_(0),
      total_banks_(header.prg_rom_chunks()) {
  assert(total_banks_ > 0);
}

void UxRom::power_on() { reset(); }
void UxRom::reset() { std::memset(mem_.prg_ram.get(), 0, mem_.prg_ram_size); }

static constexpr uint16_t CPU_BANK_0_START = 0x8000;
static constexpr uint16_t CPU_BANK_1_START = 0xc000;

static int prg_rom_addr(int bank_index, uint16_t offset) {
  return bank_index * 16 * 1024 + offset;
}

uint8_t UxRom::peek_cpu(uint16_t addr) {
  if (addr >= CPU_BANK_1_START) {
    int mapped_addr = prg_rom_addr(total_banks_ - 1, addr - CPU_BANK_1_START);
    return mem_.prg_rom[mapped_addr];
  } else if (addr >= CPU_BANK_0_START) {
    int mapped_addr = prg_rom_addr(curr_bank_, addr - CPU_BANK_0_START);
    return mem_.prg_rom[mapped_addr];
  } else {
    return 0;
  }
}

void UxRom::poke_cpu(uint16_t addr, uint8_t x) {
  if (addr >= CPU_BANK_0_START) {
    curr_bank_ = x % total_banks_;
  } else {
    // no-op
  }
}

using PeekPpu = UxRom::PeekPpu;
using PokePpu = UxRom::PokePpu;

static constexpr uint16_t PATTERN_TABLE_END = 0x2000;
static constexpr uint16_t NAME_TABLE_END    = 0x3000;

PeekPpu UxRom::peek_ppu(uint16_t addr) {
  if (addr < PATTERN_TABLE_END) {
    return PeekPpu::make_value(mem_.chr_rom[addr]);
  } else if (addr < NAME_TABLE_END) {
    return PeekPpu::make_address(mirrored_nt_addr(mirroring_, addr));
  } else {
    return PeekPpu::make_value(0); // unused
  }
}

PokePpu UxRom::poke_ppu(uint16_t addr, [[maybe_unused]] uint8_t x) {
  if (addr < PATTERN_TABLE_END) {
    if (!mem_.chr_rom_readonly) {
      mem_.chr_rom[addr] = x;
    }
    return PokePpu::make_success();
  } else if (addr < NAME_TABLE_END) {
    return PokePpu::make_address(mirrored_nt_addr(mirroring_, addr));
  } else {
    return PokePpu::make_success(); // no-op
  }
}
