#include "apu.h"
#include "cart.h"
#include "cpu.h"

#define JSON_USE_IMPLICIT_CONVERSIONS 0

#include <format>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <nlohmann/json.hpp>

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

TEST(Cpu, nestest) {
  std::ifstream is("test_data/nestest.nes", std::ios::binary);
  auto          cart = read_cart(is);
  Apu           apu;
  Cpu           cpu;

  std::ifstream log("test_data/nestest.log");

  cpu.set_cart(cart.get());
  cpu.set_apu(&apu);
  cpu.power_up();
  cpu.registers().PC = 0xc000;

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

static uint16_t to_uint16_t(const nlohmann::json &json) {
  int res = json.template get<int>();
  assert(0 <= res && res <= UINT16_MAX);
  return res;
}

static uint8_t to_uint8_t(const nlohmann::json &json) {
  int res = json.template get<int>();
  assert(0 <= res && res <= UINT8_MAX);
  return res;
}

static void single_step_test(const nlohmann::json &test_data) {
  uint8_t test_ram[64 * 1024];
  Cpu     cpu;

  cpu.set_test_ram(test_ram);
  cpu.power_up();

  auto &regs  = cpu.registers();
  auto &init  = test_data["initial"];
  auto &final = test_data["final"];

  regs.PC = to_uint16_t(init["pc"]);
  regs.S  = to_uint8_t(init["s"]);
  regs.A  = to_uint8_t(init["a"]);
  regs.X  = to_uint8_t(init["x"]);
  regs.Y  = to_uint8_t(init["y"]);
  regs.P  = to_uint8_t(init["p"]);
  for (auto &entry : init["ram"]) {
    test_ram[to_uint16_t(entry[0])] = to_uint8_t(entry[1]);
  }

  CpuCycles cycles = cpu.step();

  ASSERT_EQ(regs.PC, to_uint16_t(final["pc"]));
  ASSERT_EQ(regs.S, to_uint8_t(final["s"]));
  ASSERT_EQ(regs.A, to_uint8_t(final["a"]));
  ASSERT_EQ(regs.X, to_uint8_t(final["x"]));
  ASSERT_EQ(regs.Y, to_uint8_t(final["y"]));
  ASSERT_EQ(regs.P, to_uint8_t(final["p"]));
  for (auto &entry : final["ram"]) {
    ASSERT_EQ(test_ram[to_uint16_t(entry[0])], to_uint8_t(entry[1]));
  }

  ASSERT_EQ(cycles, test_data["cycles"].size());
}

// Single-step tests
// -----------------
//
// These are somewhat expensive to setup and run. To run these:
//
// 1. Copy the test case JSON files from
//    https://github.com/SingleStepTests/65x02/tree/main/nes6502/v1 to
//    the "test_data/single_step" directory within the source tree.
//
// 2. Remove DISABLED_ from the test name below.

class SingleStepTests : public testing::TestWithParam<uint8_t> {};

TEST_P(SingleStepTests, DISABLED_test_cases) {
  const uint8_t op_code = GetParam();
  std::string file = std::format("test_data/single_step/{:02X}.json", op_code);
  std::ifstream is(file);
  auto          test_cases = nlohmann::json::parse(is);

  for (auto &test_case : test_cases) {
    ASSERT_NO_FATAL_FAILURE(single_step_test(test_case))
        << "failed on test: " << test_case["name"] << " in " << file;
  }
}

static std::vector<uint8_t> valid_op_codes() {
  std::vector<uint8_t> op_codes;
  for (auto &op : Cpu::all_op_codes()) {
    // TODO: handle illegal opcodes in the future
    if (op.ins != Cpu::Instruction::INVALID_INS &&
        !(op.code & Cpu::OpCodeFlags::ILLEGAL)) {
      op_codes.push_back(op.code);
    }
  }
  return op_codes;
}

INSTANTIATE_TEST_SUITE_P(
    Cpu,
    SingleStepTests,
    testing::ValuesIn(valid_op_codes()),
    [](const auto &info) { return std::format("op_code_{:02X}", info.param); }
);
