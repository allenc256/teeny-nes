#pragma once

#include "src/emu/cart.h"

class NRom : public Cart {
public:
  NRom(const Header &header, Memory &&mem);

  void power_on() override;
  void reset() override;

  uint8_t peek_cpu(uint16_t addr) override;
  void    poke_cpu(uint16_t addr, uint8_t x) override;
  PeekPpu peek_ppu(uint16_t addr) override;
  PokePpu poke_ppu(uint16_t addr, uint8_t x) override;

private:
  Memory    mem_;
  uint16_t  prg_rom_mask_;
  Mirroring mirroring_;
};
