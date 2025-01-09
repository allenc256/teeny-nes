#define JSON_USE_IMPLICIT_CONVERSIONS 0

#include <format>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <nlohmann/json.hpp>

#include "src/emu/apu.h"
#include "src/emu/cart.h"
#include "src/emu/cpu.h"
#include "src/emu/ppu.h"

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

static void pad_to(std::string &out_str, size_t column) {
  while (out_str.size() < column) {
    out_str.push_back(' ');
  }
}

static std::string make_nestest_log_line(Cpu &cpu) {
  using enum Cpu::AddrMode;
  using enum Cpu::OpCodeFlags;

  std::string out_str;
  auto        out_it = std::back_inserter(out_str);
  auto       &regs   = cpu.registers();
  auto       &op     = Cpu::all_op_codes()[cpu.peek(regs.PC)];

  std::format_to(out_it, "{:04X}  ", regs.PC);

  for (uint8_t i = 0; i < 3; i++) {
    if (i < op.bytes) {
      std::format_to(out_it, "{:02X} ", cpu.peek(regs.PC + i));
    } else {
      std::format_to(out_it, "   ");
    }
  }

  pad_to(out_str, 15);
  std::format_to(
      out_it,
      "{}{} ",
      (op.flags & ILLEGAL) ? '*' : ' ',
      Cpu::instruction_names()[op.ins]
  );

  auto format_mem = [&](uint16_t addr) {
    bool io_mapped = !(addr < 0x2000 || addr >= Cart::CPU_ADDR_START);
    if (!io_mapped) {
      uint8_t mem = cpu.peek(addr);
      std::format_to(out_it, " = {:02X}", mem);
    } else {
      // don't peek IO-mapped memory, since this might have side-effects
      std::format_to(out_it, " = ??");
    }
  };

  switch (op.mode) {
  case ACCUMULATOR: std::format_to(out_it, "A"); break;
  case IMPLICIT: break;
  case ABSOLUTE: {
    uint16_t addr = cpu.decode_addr(op);
    std::format_to(out_it, "${:04X}", addr);
    if (op.ins != Cpu::JSR && op.ins != Cpu::JMP) {
      format_mem(addr);
    }
    break;
  }
  case ABSOLUTE_X: {
    uint16_t addr0 = cpu.peek16(regs.PC + 1);
    uint16_t addr1 = addr0 + regs.X;
    std::format_to(out_it, "${:04X},X @ {:04X}", addr0, addr1);
    format_mem(addr1);
    break;
  }
  case ABSOLUTE_Y: {
    uint16_t addr0 = cpu.peek16(regs.PC + 1);
    uint16_t addr1 = addr0 + regs.Y;
    std::format_to(out_it, "${:04X},Y @ {:04X}", addr0, addr1);
    format_mem(addr1);
    break;
  }
  case RELATIVE: {
    uint16_t addr = cpu.decode_addr(op);
    std::format_to(out_it, "${:04X}", addr);
    break;
  }
  case IMMEDIATE: {
    uint8_t mem = cpu.decode_mem(op);
    std::format_to(out_it, "#${:02X}", mem);
    break;
  }
  case ZERO_PAGE: {
    uint16_t addr = cpu.decode_addr(op);
    std::format_to(out_it, "${:02X}", addr);
    format_mem(addr);
    break;
  }
  case ZERO_PAGE_X: {
    uint8_t addr0 = cpu.peek(regs.PC + 1);
    uint8_t addr1 = addr0 + regs.X;
    std::format_to(out_it, "${:02X},X @ {:02X}", addr0, addr1);
    format_mem(addr1);
    break;
  }
  case ZERO_PAGE_Y: {
    uint8_t addr0 = cpu.peek(regs.PC + 1);
    uint8_t addr1 = addr0 + regs.Y;
    std::format_to(out_it, "${:02X},Y @ {:02X}", addr0, addr1);
    format_mem(addr1);
    break;
  }
  case INDIRECT: {
    uint16_t addr0 = cpu.peek16(regs.PC + 1);
    uint16_t addr  = cpu.decode_addr(op);
    std::format_to(out_it, "(${:04X}) = {:04X}", addr0, addr);
    break;
  }
  case INDIRECT_X: {
    uint8_t  a     = cpu.peek(regs.PC + 1);
    uint8_t  addr0 = a + regs.X;
    uint16_t addr  = cpu.decode_addr(op);
    std::format_to(out_it, "(${:02X},X) @ {:02X} = {:04X}", a, addr0, addr);
    format_mem(addr);
    break;
  }
  case INDIRECT_Y: {
    uint8_t  a0    = cpu.peek(regs.PC + 1);
    uint8_t  a1    = a0 + 1;
    uint16_t addr0 = (uint16_t)(cpu.peek(a0) + (cpu.peek(a1) << 8));
    uint16_t addr1 = addr0 + regs.Y;
    std::format_to(out_it, "(${:02X}),Y = {:04X} @ {:04X}", a0, addr0, addr1);
    format_mem(addr1);
    break;
  }
  default:
    throw std::runtime_error(std::format(
        "unsupported addressing mode: {} (${:02X})",
        Cpu::addr_mode_names()[op.mode],
        op.code
    ));
  }

  pad_to(out_str, 48);
  std::format_to(
      out_it,
      "A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X} PPU:???,??? CYC:{}",
      regs.A,
      regs.X,
      regs.Y,
      regs.P,
      regs.S,
      cpu.cycles()
  );

  return out_str;
}

TEST(Cpu, nestest) {
  Cart cart;
  Apu  apu;
  Cpu  cpu;
  Ppu  ppu;

  cpu.set_cart(&cart);
  cpu.set_apu(&apu);
  cpu.set_ppu(&ppu);
  apu.set_cpu(&cpu);
  ppu.set_cpu(&cpu);

  std::ifstream log("test_data/nestest.log");

  cart.load_cart("test_data/nestest.nes");
  cpu.power_on();
  cpu.registers().PC = 0xc000;

  std::string exp_line;
  while (std::getline(log, exp_line) && !exp_line.empty()) {
    std::string act_line = make_nestest_log_line(cpu);
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
  cpu.power_on();

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

  int64_t cycles_before = cpu.cycles();
  cpu.step();
  int64_t cycles_after = cpu.cycles();

  ASSERT_EQ(regs.PC, to_uint16_t(final["pc"]));
  ASSERT_EQ(regs.S, to_uint8_t(final["s"]));
  ASSERT_EQ(regs.A, to_uint8_t(final["a"]));
  ASSERT_EQ(regs.X, to_uint8_t(final["x"]));
  ASSERT_EQ(regs.Y, to_uint8_t(final["y"]));
  ASSERT_EQ(regs.P, to_uint8_t(final["p"]));
  for (auto &entry : final["ram"]) {
    ASSERT_EQ(test_ram[to_uint16_t(entry[0])], to_uint8_t(entry[1]));
  }

  ASSERT_EQ(cycles_after - cycles_before, test_data["cycles"].size());
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
