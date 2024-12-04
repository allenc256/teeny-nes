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

static void nestest_step(Cpu &cpu, const std::string &exp_line) {
  std::string act_line = cpu.disassemble();
  std::cout << act_line << '\n';
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

  std::string exp_line;
  while (std::getline(log, exp_line) && !exp_line.empty()) {
    std::string act_line = cpu.disassemble();
    ASSERT_TRUE(compare_log_lines(exp_line, act_line))
        << "expected: " << exp_line << "\nactual:   " << act_line;
    cpu.step();
  }

  ASSERT_EQ(cpu.peek(0x02), 0);
  ASSERT_EQ(cpu.peek(0x03), 0);
}
