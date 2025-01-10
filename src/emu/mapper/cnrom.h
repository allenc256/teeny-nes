#pragma once

#include "src/emu/mapper/mapper.h"

class CnRom : public Mapper {
public:
  CnRom(const CartHeader &header, CartMemory &mem);

  uint8_t peek_cpu(uint16_t addr) override;
  void    poke_cpu(uint16_t addr, uint8_t x) override;
  PeekPpu peek_ppu(uint16_t addr) override;
  PokePpu poke_ppu(uint16_t addr, uint8_t x) override;

private:
  CartMemory &mem_;
  int         bank_addr_;
  Mirroring   mirroring_;
};
