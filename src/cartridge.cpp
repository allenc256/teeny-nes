#include "cartridge.h"

#include <cassert>
#include <format>
#include <fstream>

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

  bool    has_trainer() const { return bytes_[6] & 0b00000100; }
  uint8_t prg_rom_size() const { return bytes_[4]; }

  uint8_t mapper() const {
    uint8_t lower_nibble = bytes_[6] >> 4;
    uint8_t upper_nibble = bytes_[7] & 0xf0;
    return lower_nibble | upper_nibble;
  }

private:
  static constexpr uint8_t TAG[4] = {0x4e, 0x45, 0x53, 0x1a};
  static constexpr int     SIZE   = 16;

  uint8_t bytes_[SIZE];
};

class NRom : public Cartridge {
public:
  NRom() : rom_{0} {}

  void read(std::ifstream &is, const Header &header) {
    if (header.prg_rom_size() == 1) {
      rom_mask_ = ROM_MASK_128;
    } else if (header.prg_rom_size() == 2) {
      rom_mask_ = ROM_MASK_256;
    } else {
      throw std::runtime_error(
          std::format("bad PRG ROM size: {}", header.prg_rom_size())
      );
    }

    size_t rom_bytes = header.prg_rom_size() * 16 * 1024;
    if (!is.read((char *)rom_, rom_bytes)) {
      throw std::runtime_error("failed to read PRG ROM");
    }
  }

  uint8_t peek(uint16_t address) const override {
    assert(address >= ADDR_START);
    if (address & ROM_BIT) {
      return rom_[address & rom_mask_];
    } else {
      return 0;
    }
  }

  uint8_t poke(
      [[maybe_unused]] uint16_t address, [[maybe_unused]] uint8_t value
  ) override {
    throw std::runtime_error("cannot write to read-only memory");
  }

private:
  static constexpr uint16_t ROM_BIT      = 0b1000000000000000;
  static constexpr uint16_t ROM_MASK_128 = 0b0011111111111111;
  static constexpr uint16_t ROM_MASK_256 = 0b0111111111111111;

  uint8_t  rom_[32 * 1024];
  uint16_t rom_mask_;
};

std::unique_ptr<Cartridge> read_cartridge(std::ifstream &is) {
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
