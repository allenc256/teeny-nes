#pragma once

#include "src/emu/cart.h"

class UxRom : public Cart {
public:
  UxRom(const Header &header, Memory &&mem);

  uint8_t peek_cpu(uint16_t addr) override;
  void    poke_cpu(uint16_t addr, uint8_t x) override;
  PeekPpu peek_ppu(uint16_t addr) override;
  PokePpu poke_ppu(uint16_t addr, uint8_t x) override;

private:
  Mirroring mirroring_;
  int       curr_bank_;
  int       total_banks_;
};
