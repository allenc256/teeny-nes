#include "ppu.h"

#include <format>
#include <gtest/gtest.h>

void check_internal_reg_contents(
    const Ppu::Registers &exp, const Ppu::Registers &act
) {
  ASSERT_EQ(exp.t, act.t) << std::format(
      "exp: {:016b}\nact: {:016b}", exp.t, act.t
  );
  ASSERT_EQ(exp.v, act.v) << std::format(
      "exp: {:016b}\nact: {:016b}", exp.v, act.v
  );
  ASSERT_EQ(exp.x, act.x) << std::format(
      "exp: {:016b}\nact: {:016b}", exp.x, act.x
  );
  ASSERT_EQ(exp.w, act.w) << std::format(
      "exp: {:016b}\nact: {:016b}", exp.w, act.w
  );
}

void test_internal_reg_updates(Ppu &ppu) {
  Ppu::Registers &act = ppu.registers();
  Ppu::Registers  exp = act;

  ppu.write_PPUCTRL(0b00000000);
  exp.t &= 0b111001111111111;
  check_internal_reg_contents(exp, act);

  ppu.read_PPUSTATUS();
  exp.w = 0;
  check_internal_reg_contents(exp, act);

  ppu.write_PPUSCROLL(0b01111101);
  exp.t &= 0b111111111100000;
  exp.t |= 0b000000000001111;
  exp.x = 0b101;
  exp.w = 1;
  check_internal_reg_contents(exp, act);

  ppu.write_PPUSCROLL(0b01011110);
  exp.t = 0b110000101101111;
  exp.w = 0;
  check_internal_reg_contents(exp, act);
}

TEST(Ppu, internal_register_updates) {
  Ppu ppu;

  ppu.set_ready(true);

  ppu.registers().v = 0x7fff;
  ppu.registers().t = 0x7fff;
  ppu.registers().x = 0x7;
  ppu.registers().w = 1;

  ASSERT_NO_FATAL_FAILURE(test_internal_reg_updates(ppu));

  ppu.registers().v = 0;
  ppu.registers().t = 0;
  ppu.registers().x = 0;
  ppu.registers().w = 0;

  ASSERT_NO_FATAL_FAILURE(test_internal_reg_updates(ppu));
}