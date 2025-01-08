#include <format>
#include <gtest/gtest.h>

#include "src/emu/cart.h"
#include "src/emu/ppu.h"

void check_int_regs(const Ppu::Registers &exp, const Ppu::Registers &act) {
  ASSERT_EQ(exp.t, act.t) << std::format(
      "exp.t: {:016b}\nact.t: {:016b}", exp.t, act.t
  );
  ASSERT_EQ(exp.v, act.v) << std::format(
      "exp.v: {:016b}\nact.v: {:016b}", exp.v, act.v
  );
  ASSERT_EQ(exp.x, act.x) << std::format(
      "exp.x: {:016b}\nact.x: {:016b}", exp.x, act.x
  );
  ASSERT_EQ(exp.w, act.w) << std::format(
      "exp.w: {:016b}\nact.w: {:016b}", exp.w, act.w
  );
}

// Based on diagram in https://www.nesdev.org/wiki/PPU_scrolling#Summary
void test_int_regs_1(Ppu &ppu) {
  Ppu::Registers &act = ppu.registers();
  Ppu::Registers  exp = act;

  ppu.write_PPUCTRL(0b00000000);
  exp.t &= 0b111001111111111;
  ASSERT_NO_FATAL_FAILURE(check_int_regs(exp, act));

  ppu.read_PPUSTATUS();
  exp.w = 0;
  ASSERT_NO_FATAL_FAILURE(check_int_regs(exp, act));

  ppu.write_PPUSCROLL(0b01111101);
  exp.t &= 0b111111111100000;
  exp.t |= 0b000000000001111;
  exp.x = 0b101;
  exp.w = 1;
  ASSERT_NO_FATAL_FAILURE(check_int_regs(exp, act));

  ppu.write_PPUSCROLL(0b01011110);
  exp.t = 0b110000101101111;
  exp.w = 0;
  ASSERT_NO_FATAL_FAILURE(check_int_regs(exp, act));
}

TEST(Ppu, internal_registers_1) {
  Ppu ppu;

  ppu.set_ready(true);

  ppu.registers().v = 0x7fff;
  ppu.registers().t = 0x7fff;
  ppu.registers().x = 0x7;
  ppu.registers().w = 1;

  ASSERT_NO_FATAL_FAILURE(test_int_regs_1(ppu));

  ppu.registers().v = 0;
  ppu.registers().t = 0;
  ppu.registers().x = 0;
  ppu.registers().w = 0;

  ASSERT_NO_FATAL_FAILURE(test_int_regs_1(ppu));
}

// Based on diagram in https://www.nesdev.org/wiki/PPU_scrolling#Details
static void test_int_regs_2(Ppu &ppu) {
  Ppu::Registers &act = ppu.registers();
  Ppu::Registers  exp = act;

  ASSERT_EQ(act.w, 0);

  ppu.write_PPUADDR(0b00000100);
  exp.t &= 0b000000011111111;
  exp.t |= 0b000010000000000;
  exp.w = 1;
  ASSERT_NO_FATAL_FAILURE(check_int_regs(exp, act));

  ppu.write_PPUSCROLL(0b00111110);
  exp.t &= 0b000000000011111;
  exp.t |= 0b110010011100000;
  exp.w = 0;
  ASSERT_NO_FATAL_FAILURE(check_int_regs(exp, act));

  ppu.write_PPUSCROLL(0b01111101);
  exp.t = 0b110010011101111;
  exp.x = 0b101;
  exp.w = 1;
  ASSERT_NO_FATAL_FAILURE(check_int_regs(exp, act));

  ppu.write_PPUADDR(0b11101111);
  exp.v = exp.t;
  exp.w = 0;
  ASSERT_NO_FATAL_FAILURE(check_int_regs(exp, act));
}

TEST(Ppu, internal_registers_2) {
  Ppu ppu;

  ppu.set_ready(true);

  ppu.registers().v = 0x7fff;
  ppu.registers().t = 0x7fff;
  ppu.registers().x = 0x7;
  ppu.registers().w = 0;

  ASSERT_NO_FATAL_FAILURE(test_int_regs_2(ppu));
}

TEST(Ppu, ready) {
  static constexpr int cycles_in_frame = 262 * 341;

  Cart cart;
  Ppu  ppu;

  ppu.set_cart(&cart);
  ppu.power_on();
  ASSERT_EQ(0, ppu.cycles());
  ASSERT_FALSE(ppu.ready());
  for (int i = 0; i < cycles_in_frame; i++) {
    ppu.step();
  }
  ASSERT_TRUE(ppu.ready());
}

TEST(Ppu, bg_pt_base_addr) {
  Ppu ppu;
  ppu.registers().PPUCTRL = 0;
  ASSERT_EQ(ppu.bg_pt_base_addr(), 0);
  ppu.registers().PPUCTRL = 0b00010000;
  ASSERT_EQ(ppu.bg_pt_base_addr(), 0x1000);
}

TEST(Ppu, spr_pt_base_addr) {
  Ppu ppu;
  ppu.registers().PPUCTRL = 0;
  ASSERT_EQ(ppu.spr_pt_base_addr(), 0);
  ppu.registers().PPUCTRL = 0b00001000;
  ASSERT_EQ(ppu.spr_pt_base_addr(), 0x1000);
}
