#include <cassert>
#include <format>
#include <fstream>
#include <iostream>

#include "src/emu/cart.h"

enum Mirroring { MIRROR_VERT, MIRROR_HORZ, MIRROR_ALT };

class Header {
public:
  void read(std::ifstream &is) {
    if (!is.read((char *)bytes_, SIZE)) {
      throw std::runtime_error("failed to read header");
    }
    for (size_t i = 0; i < sizeof(TAG); i++) {
      if (bytes_[i] != TAG[i]) {
        throw std::runtime_error("unsupported ROM format: unknown header type");
      }
    }
  }

  bool is_nes20_format() const {
    return (bytes_[7] & 0b00001100) == 0b00001000;
  }

  bool    has_trainer() const { return bytes_[6] & FLAGS_6_TRAINER; }
  bool    has_prg_ram() const { return bytes_[6] & FLAGS_6_PRG_RAM; }
  uint8_t prg_rom_size() const { return bytes_[4]; }
  uint8_t chr_rom_size() const { return bytes_[5]; }

  Mirroring mirroring() const {
    if (bytes_[6] & FLAGS_6_MIRROR_ALT) {
      return MIRROR_ALT;
    } else if (bytes_[6] & FLAGS_6_MIRROR_VERT) {
      return MIRROR_VERT;
    } else {
      return MIRROR_HORZ;
    }
  }

  uint8_t mapper() const {
    uint8_t lower_nibble = bytes_[6] >> 4;
    uint8_t upper_nibble = bytes_[7] & 0xf0;
    return lower_nibble | upper_nibble;
  }

private:
  static constexpr uint8_t TAG[4] = {0x4e, 0x45, 0x53, 0x1a};
  static constexpr int     SIZE   = 16;

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

  uint8_t bytes_[SIZE];
};

class NRom : public Cart {
public:
  NRom() : prg_ram_{0} {}

  void read(std::ifstream &is, const Header &header) {
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

    if (header.mirroring() == MIRROR_HORZ ||
        header.mirroring() == MIRROR_VERT) {
      mirroring_ = header.mirroring();
    } else {
      throw std::runtime_error("bad mirroring type");
    }
  }

  uint8_t peek_cpu(uint16_t addr) override {
    assert(addr >= CPU_ADDR_START);
    if (addr & PRG_ROM_BIT) {
      return prg_rom_[addr & prg_rom_mask_];
    } else {
      return 0;
    }
  }

  void poke_cpu(uint16_t addr, uint8_t x) override {
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

  PeekPpu peek_ppu(uint16_t addr) override {
    assert(addr < UNUSED_END);
    if (addr < PATTERN_TABLE_END) {
      return PeekPpu::make_value(chr_mem_[addr]);
    } else if (addr < NAME_TABLE_END) {
      return PeekPpu::make_address(map_name_table_addr(addr));
    } else {
      return PeekPpu::make_value(0); // unused
    }
  }

  PokePpu poke_ppu(uint16_t addr, uint8_t x) override {
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

private:
  uint16_t map_name_table_addr(uint16_t addr) {
    if (mirroring_ == MIRROR_HORZ) {
      return (addr & MIRROR_HORZ_OFF_MASK) |
             ((addr & MIRROR_HORZ_NT_MASK) >> 1);
    } else {
      return addr & MIRROR_VERT_MASK;
    }
  }

  static constexpr uint16_t PRG_ROM_BIT          = 0b1000000000000000;
  static constexpr uint16_t PRG_ROM_MASK_128     = 0b0011111111111111;
  static constexpr uint16_t PRG_ROM_MASK_256     = 0b0111111111111111;
  static constexpr uint16_t PATTERN_TABLE_END    = 0x2000;
  static constexpr uint16_t NAME_TABLE_END       = 0x3000;
  static constexpr uint16_t UNUSED_END           = 0x3f00;
  static constexpr uint16_t MIRROR_HORZ_OFF_MASK = 0x3ff;
  static constexpr uint16_t MIRROR_HORZ_NT_MASK  = 0x800;
  static constexpr uint16_t MIRROR_VERT_MASK     = 0x7ff;

  uint8_t   prg_rom_[32 * 1024];
  uint8_t   prg_ram_[8 * 1024];
  uint8_t   chr_mem_[8 * 1024];
  bool      prg_ram_enabled_;
  bool      chr_mem_readonly_;
  uint16_t  prg_rom_mask_;
  Mirroring mirroring_;
};

std::unique_ptr<Cart> read_cart(std::ifstream &is) {
  Header header;

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
