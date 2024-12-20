#include <cassert>
#include <format>
#include <fstream>
#include <iostream>

#include "src/emu/cart.h"
#include "src/emu/mapper/nrom.h"

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
static constexpr uint8_t FLAGS_6_MIRROR_VERT = 0b00000001;
static constexpr uint8_t FLAGS_6_PRG_RAM     = 0b00000010;
static constexpr uint8_t FLAGS_6_TRAINER     = 0b00000100;
static constexpr uint8_t FLAGS_6_MIRROR_ALT  = 0b00001000;

void INesHeader::read(std::ifstream &is) {
  if (!is.read((char *)bytes_, sizeof(bytes_))) {
    throw std::runtime_error("failed to read header");
  }
  for (size_t i = 0; i < sizeof(HEADER_TAG); i++) {
    if (bytes_[i] != HEADER_TAG[i]) {
      throw std::runtime_error("unsupported ROM format: unknown header type");
    }
  }
}

bool INesHeader::is_nes20_format() const {
  return (bytes_[7] & 0b00001100) == 0b00001000;
}

bool    INesHeader::has_trainer() const { return bytes_[6] & FLAGS_6_TRAINER; }
bool    INesHeader::has_prg_ram() const { return bytes_[6] & FLAGS_6_PRG_RAM; }
uint8_t INesHeader::prg_rom_size() const { return bytes_[4]; }
uint8_t INesHeader::chr_rom_size() const { return bytes_[5]; }

Mirroring INesHeader::mirroring() const {
  if (bytes_[6] & FLAGS_6_MIRROR_ALT) {
    return MIRROR_ALT;
  } else if (bytes_[6] & FLAGS_6_MIRROR_VERT) {
    return MIRROR_VERT;
  } else {
    return MIRROR_HORZ;
  }
}

uint8_t INesHeader::mapper() const {
  uint8_t lower_nibble = bytes_[6] >> 4;
  uint8_t upper_nibble = bytes_[7] & 0xf0;
  return lower_nibble | upper_nibble;
}

std::unique_ptr<Cart> read_cart(std::ifstream &is) {
  INesHeader header;

  header.read(is);

  if (header.is_nes20_format()) {
    throw std::runtime_error("unsupported ROM format: NES 2.0");
  }
  if (header.has_trainer()) {
    throw std::runtime_error("unsupported ROM format: trainer");
  }

  if (header.mapper() == 0) {
    auto rom = std::make_unique<NRom>();
    rom->read(is, header);
    return rom;
  } else {
    throw std::runtime_error(
        std::format("unsupported ROM format: mapper {}", header.mapper())
    );
  }
}
