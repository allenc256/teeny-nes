#include "cpu.h"

#include <format>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

static bool compare_log_lines(const std::string &exp, const std::string &act) {
  if (exp.size() != act.size()) {
    return false;
  }
  for (size_t i = 0; i < exp.size(); i++) {
    if (act[i] != '?' && act[i] != exp[i]) {
      return false;
    }
  }
  return true;
}

static void nestest_step(Cpu &cpu, std::istream &exp_log) {
  std::string act_line = cpu.disassemble();
  std::cout << act_line << '\n';

  std::string exp_line;
  ASSERT_TRUE(std::getline(exp_log, exp_line));

  ASSERT_TRUE(compare_log_lines(exp_line, act_line))
      << "expected: " << exp_line << "\nactual:   " << act_line;

  cpu.step();
}

TEST(Cpu, nestest) {
  std::ifstream is("test_data/nestest.nes", std::ios::binary);
  auto          cart = read_cartridge(is);
  Cpu           cpu(*cart);

  std::ifstream log("test_data/nestest.log");

  cpu.registers().PC = 0x0c000;
  cpu.set_cycles(7);

  for (int i = 0; i < 10000; i++) {
    ASSERT_NO_FATAL_FAILURE(nestest_step(cpu, log));
  }
}
