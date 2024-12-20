#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "src/emu/mapper/nrom.h"

NRom::NRom() : prg_ram_{0} {}

void NRom::read(std::ifstream &is, const INesHeader &header) {
  if (header.prg_rom_size() == 1) {
    prg_rom_mask_ = PRG_ROM_MASK_128;
  } else if (header.prg_rom_size() == 2) {
    prg_rom_mask_ = PRG_ROM_MASK_256;
  } else {
    throw std::runtime_error(
        std::format("bad PRG ROM size: {}", header.prg_rom_size())
    );
  }

  if (header.chr_rom_size() == 0) {
    chr_mem_readonly_ = false;
  } else if (header.chr_rom_size() == 1) {
    chr_mem_readonly_ = true;
  } else {
    throw std::runtime_error(
        std::format("bad CHR ROM size: {}", header.chr_rom_size())
    );
  }

  prg_ram_enabled_ = header.has_prg_ram();

  size_t prg_rom_bytes = header.prg_rom_size() * 16 * 1024;
  if (!is.read((char *)prg_rom_, prg_rom_bytes)) {
    throw std::runtime_error("failed to read PRG ROM");
  }

  size_t chr_rom_bytes = header.chr_rom_size() * 8 * 1024;
  if (!is.read((char *)chr_mem_, chr_rom_bytes)) {
    throw std::runtime_error("failed to read CHR ROM");
  }

  if (header.mirroring() == MIRROR_HORZ || header.mirroring() == MIRROR_VERT) {
    mirroring_ = header.mirroring();
  } else {
    throw std::runtime_error("bad mirroring type");
  }
}

uint8_t NRom::peek_cpu(uint16_t addr) {
  assert(addr >= CPU_ADDR_START);
  if (addr & PRG_ROM_BIT) {
    return prg_rom_[addr & prg_rom_mask_];
  } else {
    return 0;
  }
}

void NRom::poke_cpu(uint16_t addr, uint8_t x) {
  if (prg_ram_enabled_ && addr >= 0x6000 && addr <= 0x7fff) {
    prg_ram_[addr - 0x6000] = x;
  } else {
    std::cerr << std::format(
        "WARNING: ignoring attempted write by CPU to read-only memory: "
        "${:04x}\n",
        addr
    );
  }
}

NRom::PeekPpu NRom::peek_ppu(uint16_t addr) {
  assert(addr < UNUSED_END);
  if (addr < PATTERN_TABLE_END) {
    return PeekPpu::make_value(chr_mem_[addr]);
  } else if (addr < NAME_TABLE_END) {
    return PeekPpu::make_address(map_name_table_addr(addr));
  } else {
    return PeekPpu::make_value(0); // unused
  }
}

NRom::PokePpu NRom::poke_ppu(uint16_t addr, uint8_t x) {
  assert(addr < UNUSED_END);
  if (addr < PATTERN_TABLE_END) {
    if (chr_mem_readonly_) {
      std::cerr << std::format(
          "WARNING: ignoring attempted write by PPU to read-only memory: "
          "${:04x}\n",
          addr
      );
    } else {
      chr_mem_[addr] = x;
    }
    return PokePpu::make_success();
  } else if (addr < NAME_TABLE_END) {
    return PokePpu::make_address(map_name_table_addr(addr));
  } else {
    return PokePpu::make_success(); // no-op
  }
}

uint16_t NRom::map_name_table_addr(uint16_t addr) {
  if (mirroring_ == MIRROR_HORZ) {
    return (addr & MIRROR_HORZ_OFF_MASK) | ((addr & MIRROR_HORZ_NT_MASK) >> 1);
  } else {
    return addr & MIRROR_VERT_MASK;
  }
}
