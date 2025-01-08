#include <cassert>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>

#include "src/emu/cart.h"
#include "src/emu/mapper/mmc1.h"
#include "src/emu/mapper/mmc3.h"
#include "src/emu/mapper/nrom.h"
#include "src/emu/mapper/uxrom.h"

CartHeader read_header(std::ifstream &is) {
  uint8_t bytes[16];
  if (!is.read((char *)bytes, sizeof(bytes))) {
    throw std::runtime_error("failed to read header");
  }
  return CartHeader(bytes);
}

CartMemory read_data(std::ifstream &is, const CartHeader &header) {
  CartMemory mem;

  if (header.has_trainer()) {
    throw std::runtime_error("unsupported ROM format: trainer");
  }

  mem.prg_rom_size = header.prg_rom_chunks() * 16 * 1024;
  mem.prg_rom      = std::make_unique<uint8_t[]>(mem.prg_rom_size);
  if (!is.read((char *)mem.prg_rom.get(), mem.prg_rom_size)) {
    throw std::runtime_error(
        std::format("failed to read PRG ROM: {} bytes", mem.prg_rom_size)
    );
  }

  if (header.chr_rom_readonly()) {
    mem.chr_rom_size     = header.chr_rom_chunks() * 8 * 1024;
    mem.chr_rom_readonly = true;
    mem.chr_rom          = std::make_unique<uint8_t[]>(mem.chr_rom_size);
    if (!is.read((char *)mem.chr_rom.get(), mem.chr_rom_size)) {
      throw std::runtime_error(
          std::format("failed to read CHR ROM: {} bytes", mem.chr_rom_size)
      );
    }
  } else {
    mem.chr_rom_size     = 8 * 1024;
    mem.chr_rom_readonly = false;
    mem.chr_rom          = std::make_unique<uint8_t[]>(mem.chr_rom_size);
    std::memset(mem.chr_rom.get(), 0, mem.chr_rom_size);
  }

  mem.prg_ram_size = std::max(1, header.prg_ram_chunks()) * 8 * 1024;
  mem.prg_ram      = std::make_unique<uint8_t[]>(mem.prg_ram_size);
  std::memset(mem.prg_ram.get(), 0, mem.prg_ram_size);

  mem.prg_ram_persistent = header.prg_ram_persistent();

  return mem;
}

static void save_prg_ram(const CartMemory &mem) {
  if (!mem.prg_ram_persistent || mem.prg_ram_save_path.empty()) {
    return;
  }

  std::ofstream os(mem.prg_ram_save_path, std::ios::binary);
  if (!os) {
    std::cerr << std::format(
        "failed to open PRG RAM file for writing: {}\n",
        mem.prg_ram_save_path.c_str()
    );
    return;
  }

  if (!os.write((const char *)mem.prg_ram.get(), mem.prg_ram_size)) {
    std::cerr << std::format(
        "failed to write PRG RAM file: {}\n", mem.prg_ram_save_path.c_str()
    );
    return;
  }
}

static void load_prg_ram(CartMemory &mem) {
  if (!mem.prg_ram_persistent || mem.prg_ram_save_path.empty()) {
    return;
  }
  if (!std::filesystem::exists(mem.prg_ram_save_path)) {
    return;
  }

  std::ifstream is(mem.prg_ram_save_path, std::ios::binary);
  if (!is) {
    std::cerr << std::format(
        "failed to open PRG RAM file for reading: {}\n",
        mem.prg_ram_save_path.c_str()
    );
    return;
  }

  if (!is.read((char *)mem.prg_ram.get(), mem.prg_ram_size)) {
    std::cerr << std::format(
        "failed to read PRG RAM file: {}\n", mem.prg_ram_save_path.c_str()
    );
    return;
  }
}

bool Cart::loaded() const { return mapper_.get() != nullptr; }
void Cart::power_on() { mapper_->power_on(); }

void Cart::power_off() {
  gg_codes_.clear();
  save_prg_ram(mem_);
}

void Cart::reset() {
  save_prg_ram(mem_);
  mapper_->reset();
}

void Cart::step_ppu() {
  if (step_ppu_enabled_) {
    mapper_->step_ppu();
  }
}

uint8_t Cart::peek_cpu(uint16_t addr) {
  assert(addr >= CPU_ADDR_START);

  uint8_t x = mapper_->peek_cpu(addr);

  if (!gg_codes_.empty()) {
    for (auto &code : gg_codes_) {
      if (code.applies(addr, x)) {
        return code.value();
      }
    }
  }

  return x;
}

void Cart::poke_cpu(uint16_t addr, uint8_t x) {
  assert(addr >= CPU_ADDR_START);
  mapper_->poke_cpu(addr, x);
}

PeekPpu Cart::peek_ppu(uint16_t addr) {
  assert(addr < PPU_ADDR_END);
  return mapper_->peek_ppu(addr);
}

PokePpu Cart::poke_ppu(uint16_t addr, uint8_t x) {
  assert(addr < PPU_ADDR_END);
  return mapper_->poke_ppu(addr, x);
}

void Cart::load_cart(const std::filesystem::path &path) {
  std::ifstream is(path, std::ios::binary);
  if (!is) {
    throw std::runtime_error(
        std::format("failed to open file: {}", path.c_str())
    );
  }

  CartHeader header = read_header(is);
  mem_              = read_data(is, header);

  if (header.prg_ram_persistent()) {
    mem_.prg_ram_save_path = path;
    mem_.prg_ram_save_path.replace_extension(".sav");
  }

  load_prg_ram(mem_);

  switch (header.mapper()) {
  case 0: mapper_ = std::make_unique<NRom>(header, mem_); break;
  case 1: mapper_ = std::make_unique<Mmc1>(mem_); break;
  case 2: mapper_ = std::make_unique<UxRom>(header, mem_); break;
  case 4: mapper_ = std::make_unique<Mmc3>(header, mem_, *cpu_, *ppu_); break;
  default:
    throw std::runtime_error(
        std::format("unsupported ROM format: mapper {}", header.mapper())
    );
  }

  step_ppu_enabled_ = mapper_->step_ppu_enabled();
}
