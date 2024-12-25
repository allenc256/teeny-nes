#pragma once

#include "src/emu/cart.h"

class Mmc3 : public Cart {
public:
  Mmc3(const Header &header, Memory &&mem);

  void set_cpu(Cpu *cpu) override;
  void set_ppu(Ppu *ppu) override;
  void power_on() override;
  void reset() override;

  uint8_t peek_cpu(uint16_t addr) override;
  void    poke_cpu(uint16_t addr, uint8_t x) override;
  PeekPpu peek_ppu(uint16_t addr) override;
  PokePpu poke_ppu(uint16_t addr, uint8_t x) override;

  bool step_ppu() override;

private:
  struct Registers {
    uint8_t R[8];
    uint8_t bank_select;
  };

  struct IrqCounter {
    uint8_t latch;
    uint8_t counter;
    bool    enabled;
    bool    reload;
    bool    prev_a12;
    int64_t prev_cycles;
  };

  void write_bank_data(uint8_t x);
  void write_mirroring(uint8_t x);

  int prg_rom_banks() const;
  int chr_rom_banks() const;

  int map_prg_rom_addr(uint16_t cpu_addr);
  int map_chr_rom_addr(uint16_t ppu_addr);

  void clock_IRQ_counter();

  Memory     mem_;
  Registers  regs_;
  IrqCounter irq_;
  Cpu       *cpu_;
  Ppu       *ppu_;
  Mirroring  mirroring_;
  Mirroring  orig_mirroring_;
};
