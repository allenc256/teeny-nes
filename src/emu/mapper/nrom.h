#pragma once

#include "src/emu/cart.h"

class NRom : public Mapper {
public:
  NRom(const CartHeader &header, CartMemory &mem);

  uint8_t peek_cpu(uint16_t addr) override;
  void    poke_cpu(uint16_t addr, uint8_t x) override;
  PeekPpu peek_ppu(uint16_t addr) override;
  PokePpu poke_ppu(uint16_t addr, uint8_t x) override;

private:
  CartMemory &mem_;
  uint16_t    prg_rom_mask_;
  Mirroring   mirroring_;
};
