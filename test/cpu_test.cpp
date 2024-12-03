#include "cpu.h"

#include <format>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

static bool compare_log_lines(const std::string &exp, const std::string &act) {
  if (exp.size() + 1 != act.size()) {
    return false;
  }
  for (size_t i = 0; i < exp.size(); i++) {
    if (act[i] != '?' && act[i] != exp[i]) {
      return false;
    }
  }
  return true;
}

static void
test_step(Cpu &cpu, std::istream &exp_log, std::ostringstream &act_log) {
  act_log.clear();
  act_log.seekp(0);

  cpu.step();

  std::string exp_line;
  ASSERT_TRUE(std::getline(exp_log, exp_line));
  ASSERT_TRUE(compare_log_lines(exp_line, act_log.str()))
      << "expected: " << exp_line << "\nactual:   " << act_log.str();

  std::cout << act_log.str();
}

TEST(Cpu, nestest) {
  std::ifstream is("test_data/nestest.nes", std::ios::binary);
  auto          cart = read_cartridge(is);
  Cpu           cpu(*cart);

  std::ifstream      exp_log("test_data/nestest.log");
  std::ostringstream act_log;

  cpu.enable_logging(&act_log);
  cpu.registers().PC = 0x0c000;
  cpu.set_cycles(7);

  for (int i = 0; i < 80; i++) {
    ASSERT_NO_FATAL_FAILURE(test_step(cpu, exp_log, act_log));
  }
}
