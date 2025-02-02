#pragma once

#include "src/emu/cart.h"

class Mmc1 : public Mapper {
public:
  Mmc1(CartMemory &mem);

  void power_on() override;
  void reset() override;

  uint8_t peek_cpu(uint16_t addr) override;
  void    poke_cpu(uint16_t addr, uint8_t x) override;
  PeekPpu peek_ppu(uint16_t addr) override;
  PokePpu poke_ppu(uint16_t addr, uint8_t x) override;

private:
  struct Registers {
    uint8_t shift;
    uint8_t control;
    uint8_t chr_bank_0;
    uint8_t chr_bank_1;
    uint8_t prg_bank;
  };

  void write_shift_reg(uint16_t addr, uint8_t x);

  int prg_rom_banks() const;

  int map_prg_rom_addr(uint16_t addr) const;
  int map_chr_rom_addr(uint16_t addr) const;

  Mirroring mirroring() const;

  Registers   regs_;
  CartMemory &mem_;
};
