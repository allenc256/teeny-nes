#include <cstring>
#include <stdexcept>

#include "src/emu/mapper/mapper.h"

static constexpr uint16_t MIRROR_HORZ_OFF_MASK = 0x3ff;
static constexpr uint16_t MIRROR_HORZ_NT_MASK  = 0x800;
static constexpr uint16_t MIRROR_VERT_MASK     = 0x7ff;

uint16_t Mapper::mirrored_nt_addr(Mirroring mirroring, uint16_t addr) {
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

CartHeader::CartHeader(uint8_t bytes[16]) {
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

bool CartHeader::has_trainer() const { return bytes_[6] & FLAGS_6_TRAINER; }

bool CartHeader::prg_ram_persistent() const {
  return bytes_[6] & FLAGS_6_PRG_RAM_PERSISTENT;
}

int  CartHeader::prg_rom_chunks() const { return bytes_[4]; }
int  CartHeader::chr_rom_chunks() const { return bytes_[5]; }
int  CartHeader::prg_ram_chunks() const { return bytes_[8]; }
bool CartHeader::chr_rom_readonly() const { return chr_rom_chunks() > 0; }

bool CartHeader::mirroring_specified() const {
  return !(bytes_[6] & FLAGS_6_MIRROR_ALT);
}

Mirroring CartHeader::mirroring() const {
  if (!mirroring_specified()) {
    throw std::runtime_error("mirroring not specified");
  } else if (bytes_[6] & FLAGS_6_MIRROR_VERT) {
    return MIRROR_VERT;
  } else {
    return MIRROR_HORZ;
  }
}

int CartHeader::mapper() const {
  uint8_t lower_nibble = bytes_[6] >> 4;
  uint8_t upper_nibble = bytes_[7] & 0xf0;
  return lower_nibble | upper_nibble;
}
