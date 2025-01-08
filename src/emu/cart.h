#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "src/emu/game_genie.h"
#include "src/emu/mapper/mapper.h"

class Cart {
public:
  static constexpr uint16_t PPU_ADDR_END   = 0x3f00;
  static constexpr uint16_t CPU_ADDR_START = 0x4020;

  void load_cart(const std::filesystem::path &path);
  bool loaded() const;

  void set_cpu(Cpu *cpu) { cpu_ = cpu; }
  void set_ppu(Ppu *ppu) { ppu_ = ppu; }

  void power_on();
  void power_off();
  void reset();

  uint8_t peek_cpu(uint16_t addr);
  void    poke_cpu(uint16_t addr, uint8_t x);

  PeekPpu peek_ppu(uint16_t addr);
  PokePpu poke_ppu(uint16_t addr, uint8_t x);

  void step_ppu();

  std::vector<GameGenieCode> &gg_codes() { return gg_codes_; }

private:
  CartMemory                 mem_;
  std::unique_ptr<Mapper>    mapper_;
  Cpu                       *cpu_              = nullptr;
  Ppu                       *ppu_              = nullptr;
  bool                       step_ppu_enabled_ = false;
  std::vector<GameGenieCode> gg_codes_;
};
