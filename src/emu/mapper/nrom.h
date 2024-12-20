#include "src/emu/cart.h"

class NRom : public Cart {
public:
  NRom();

  void    read(std::ifstream &is, const INesHeader &header);
  uint8_t peek_cpu(uint16_t addr) override;
  void    poke_cpu(uint16_t addr, uint8_t x) override;
  PeekPpu peek_ppu(uint16_t addr) override;
  PokePpu poke_ppu(uint16_t addr, uint8_t x) override;

private:
  uint16_t map_name_table_addr(uint16_t addr);

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
