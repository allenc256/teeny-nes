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

using Header    = Cart::Header;
using Mirroring = Cart::Mirroring;
using Memory    = Cart::Memory;

static constexpr uint16_t MIRROR_HORZ_OFF_MASK = 0x3ff;
static constexpr uint16_t MIRROR_HORZ_NT_MASK  = 0x800;
static constexpr uint16_t MIRROR_VERT_MASK     = 0x7ff;

uint16_t Cart::mirrored_nt_addr(Mirroring mirroring, uint16_t addr) {
  switch (mirroring) {
  case MIRROR_HORZ:
    return (addr & MIRROR_HORZ_OFF_MASK) | ((addr & MIRROR_HORZ_NT_MASK) >> 1);
  case MIRROR_VERT: return addr & MIRROR_VERT_MASK;
  case MIRROR_SCREEN_A_ONLY: return addr & 0x3ff;
  case MIRROR_SCREEN_B_ONLY: return (addr & 0x3ff) + 1024;
  default: throw std::runtime_error("unreachable");
  }
}

static constexpr uint8_t HEADER_TAG[4] = {0x4e, 0x45, 0x53, 0x1a};

// Flags 6
// =======
//
// 76543210
// ||||||||
// |||||||+- Nametable arrangement: 0: vertical
// |||||||                          1: horizontal
// ||||||+-- 1: Cartridge contains battery-backed PRG RAM ($6000-7FFF)
// |||||+--- 1: 512-byte trainer at $7000-$71FF (stored before PRG data)
// ||||+---- 1: Alternative nametable layout
// ++++----- Lower nybble of mapper number
static constexpr uint8_t FLAGS_6_MIRROR_VERT        = 0b00000001;
static constexpr uint8_t FLAGS_6_PRG_RAM_PERSISTENT = 0b00000010;
static constexpr uint8_t FLAGS_6_TRAINER            = 0b00000100;
static constexpr uint8_t FLAGS_6_MIRROR_ALT         = 0b00001000;

static bool header_is_nes20(uint8_t bytes[16]) {
  return (bytes[7] & 0b00001100) == 0b00001000;
}

Header::Header(uint8_t bytes[16]) {
  for (size_t i = 0; i < sizeof(HEADER_TAG); i++) {
    if (bytes[i] != HEADER_TAG[i]) {
      throw std::runtime_error("unsupported ROM format: unknown header type");
    }
  }
  if (header_is_nes20(bytes)) {
    throw std::runtime_error("NES 2.0 headers not supported");
  }

  std::memcpy(bytes_, bytes, sizeof(bytes_));
}

bool Header::has_trainer() const { return bytes_[6] & FLAGS_6_TRAINER; }

bool Header::prg_ram_persistent() const {
  return bytes_[6] & FLAGS_6_PRG_RAM_PERSISTENT;
}

int  Header::prg_rom_chunks() const { return bytes_[4]; }
int  Header::chr_rom_chunks() const { return bytes_[5]; }
int  Header::prg_ram_chunks() const { return bytes_[8]; }
bool Header::chr_rom_readonly() const { return chr_rom_chunks() > 0; }

bool Header::mirroring_specified() const {
  return !(bytes_[6] & FLAGS_6_MIRROR_ALT);
}

Mirroring Header::mirroring() const {
  if (!mirroring_specified()) {
    throw std::runtime_error("mirroring not specified");
  } else if (bytes_[6] & FLAGS_6_MIRROR_VERT) {
    return MIRROR_VERT;
  } else {
    return MIRROR_HORZ;
  }
}

int Header::mapper() const {
  uint8_t lower_nibble = bytes_[6] >> 4;
  uint8_t upper_nibble = bytes_[7] & 0xf0;
  return lower_nibble | upper_nibble;
}

Header read_header(std::ifstream &is) {
  uint8_t bytes[16];
  if (!is.read((char *)bytes, sizeof(bytes))) {
    throw std::runtime_error("failed to read header");
  }
  return Header(bytes);
}

Memory read_data(std::ifstream &is, const Header &header) {
  Memory mem;

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

Cart::Cart(Memory &&mem) : mem_(std::move(mem)) {}

static void save_prg_ram(const Memory &mem) {
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

static void load_prg_ram(Memory &mem) {
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

  std::cout << std::format(
      "read {} bytes from {}\n", mem.prg_ram_size, mem.prg_ram_save_path.c_str()
  );
}

void Cart::power_on() { internal_power_on(); }
void Cart::power_off() { save_prg_ram(mem_); }

void Cart::reset() {
  save_prg_ram(mem_);
  internal_reset();
}

std::unique_ptr<Cart> read_cart(const std::filesystem::path &path) {
  std::ifstream is(path, std::ios::binary);
  if (!is) {
    throw std::runtime_error(
        std::format("failed to open file: {}", path.c_str())
    );
  }

  Header header = read_header(is);
  Memory mem    = read_data(is, header);

  if (header.prg_ram_persistent()) {
    mem.prg_ram_save_path = path;
    mem.prg_ram_save_path.replace_extension(".sav");
  }

  load_prg_ram(mem);

  std::unique_ptr<Cart> cart;
  switch (header.mapper()) {
  case 0: return std::make_unique<NRom>(header, std::move(mem));
  case 1: return std::make_unique<Mmc1>(header, std::move(mem));
  case 2: return std::make_unique<UxRom>(header, std::move(mem));
  case 4: return std::make_unique<Mmc3>(header, std::move(mem));
  default:
    throw std::runtime_error(
        std::format("unsupported ROM format: mapper {}", header.mapper())
    );
  }
}
