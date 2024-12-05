#pragma once

#include "bus.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>

enum Instruction : uint8_t {
  ADC,
  AND,
  ASL,
  BCC,
  BCS,
  BEQ,
  BIT,
  BMI,
  BNE,
  BPL,
  BRA,
  BRK,
  BVC,
  BVS,
  CLC,
  CLD,
  CLI,
  CLV,
  CMP,
  CPX,
  CPY,
  DCP,
  DEC,
  DEX,
  DEY,
  EOR,
  INC,
  INX,
  INY,
  ISB,
  JMP,
  JSR,
  LAX,
  LDA,
  LDX,
  LDY,
  LSR,
  NOP,
  ORA,
  PHA,
  PHP,
  PHX,
  PHY,
  PLA,
  PLP,
  PLX,
  PLY,
  RLA,
  ROL,
  ROR,
  RRA,
  RTI,
  RTS,
  SAX,
  SBC,
  SEC,
  SED,
  SEI,
  SLO,
  SRE,
  STA,
  STX,
  STY,
  STZ,
  TAX,
  TAY,
  TRB,
  TSB,
  TSX,
  TXA,
  TXS,
  TYA,
  INVALID_INS
};

enum AddrMode : uint8_t {
  ABSOLUTE,
  ABSOLUTE_X,
  ABSOLUTE_Y,
  ACCUMULATOR,
  IMMEDIATE,
  IMPLICIT,
  INDIRECT,
  INDIRECT_X,
  INDIRECT_Y,
  RELATIVE,
  ZERO_PAGE,
  ZERO_PAGE_X,
  ZERO_PAGE_Y,
  INVALID_ADDR_MODE,
};

enum OpCodeFlags : uint8_t {
  ILLEGAL    = 1u << 0,
  FORCE_OOPS = 1u << 1,
};

struct OpCode {
  uint8_t     code        = 0;
  Instruction ins         = INVALID_INS;
  AddrMode    mode        = INVALID_ADDR_MODE;
  uint8_t     bytes       = 0;
  uint8_t     base_cycles = 0;
  uint8_t     flags       = 0;
};

enum Flags : uint8_t {
  C_FLAG     = 1u << 0,
  Z_FLAG     = 1u << 1,
  I_FLAG     = 1u << 2,
  D_FLAG     = 1u << 3,
  B_FLAG     = 1u << 4,
  DUMMY_FLAG = 1u << 5,
  V_FLAG     = 1u << 6,
  N_FLAG     = 1u << 7,
};

struct Registers {
  uint16_t PC;
  uint8_t  S;
  uint8_t  A;
  uint8_t  X;
  uint8_t  Y;
  uint8_t  P;
};

class Cpu {
public:
  Cpu(BusBase &bus);

  const Registers &registers() const { return regs_; }
  Registers       &registers() { return regs_; }
  int64_t          cycles() const { return cycles_; }
  void             set_cycles(int64_t cycles) { cycles_ = cycles; }

  uint8_t  peek(uint16_t addr) { return bus_.peek(addr); }
  void     poke(uint16_t addr, uint8_t value) { return bus_.poke(addr, value); }
  void     push8(uint8_t x);
  void     push16(uint16_t x);
  uint8_t  pop8();
  uint16_t pop16();
  void     step();
  std::string disassemble();

  static const std::array<OpCode, 256> &all_op_codes();

private:
  void power_up();
  void reset();

  void step_ADC(const OpCode &op);
  void step_AND(const OpCode &op);
  void step_ASL(const OpCode &op);
  void step_BCC(const OpCode &op);
  void step_BCS(const OpCode &op);
  void step_BEQ(const OpCode &op);
  void step_BIT(const OpCode &op);
  void step_BMI(const OpCode &op);
  void step_BNE(const OpCode &op);
  void step_BPL(const OpCode &op);
  void step_BRK(const OpCode &op);
  void step_BVC(const OpCode &op);
  void step_BVS(const OpCode &op);
  void step_CLC(const OpCode &op);
  void step_CLD(const OpCode &op);
  void step_CLI(const OpCode &op);
  void step_CLV(const OpCode &op);
  void step_CMP(const OpCode &op);
  void step_CPX(const OpCode &op);
  void step_CPY(const OpCode &op);
  void step_DCP(const OpCode &op);
  void step_DEC(const OpCode &op);
  void step_DEX(const OpCode &op);
  void step_DEY(const OpCode &op);
  void step_EOR(const OpCode &op);
  void step_INC(const OpCode &op);
  void step_INX(const OpCode &op);
  void step_INY(const OpCode &op);
  void step_ISB(const OpCode &op);
  void step_JMP(const OpCode &op);
  void step_JSR(const OpCode &op);
  void step_LAX(const OpCode &op);
  void step_LDA(const OpCode &op);
  void step_LDX(const OpCode &op);
  void step_LDY(const OpCode &op);
  void step_LSR(const OpCode &op);
  void step_NOP(const OpCode &op);
  void step_ORA(const OpCode &op);
  void step_PHA(const OpCode &op);
  void step_PHP(const OpCode &op);
  void step_PHX(const OpCode &op);
  void step_PHY(const OpCode &op);
  void step_PLA(const OpCode &op);
  void step_PLP(const OpCode &op);
  void step_PLX(const OpCode &op);
  void step_PLY(const OpCode &op);
  void step_RLA(const OpCode &op);
  void step_ROL(const OpCode &op);
  void step_ROR(const OpCode &op);
  void step_RRA(const OpCode &op);
  void step_RTI(const OpCode &op);
  void step_RTS(const OpCode &op);
  void step_SAX(const OpCode &op);
  void step_SBC(const OpCode &op);
  void step_SEC(const OpCode &op);
  void step_SED(const OpCode &op);
  void step_SEI(const OpCode &op);
  void step_SLO(const OpCode &op);
  void step_SRE(const OpCode &op);
  void step_STA(const OpCode &op);
  void step_STX(const OpCode &op);
  void step_STY(const OpCode &op);
  void step_TAX(const OpCode &op);
  void step_TAY(const OpCode &op);
  void step_TSX(const OpCode &op);
  void step_TXA(const OpCode &op);
  void step_TXS(const OpCode &op);
  void step_TYA(const OpCode &op);

  void step_load_mem(const OpCode &op, uint8_t &reg);
  void step_load_stack(uint8_t &reg);
  void step_load(uint8_t res, uint8_t &reg);
  void step_compare(const OpCode &op, uint8_t &reg);
  void step_branch(const OpCode &op, bool test);
  void step_shift_left(const OpCode &op, bool carry);
  void step_shift_right(const OpCode &op, bool carry);
  void step_unimplemented(const OpCode &op);

  void set_flag(Flags flag, bool value);

  uint16_t decode_addr(const OpCode &op);
  uint8_t  decode_mem(const OpCode &op);

  static constexpr int RAM_SIZE    = 0x0800;
  static constexpr int RAM_MASK    = 0x07ff;
  static constexpr int STACK_START = 0x0100;
  static constexpr int RAM_END     = 0x2000;

  Registers regs_;
  Apu       apu_;
  BusBase  &bus_;
  uint64_t  cycles_;
  bool      oops_;
  bool      jump_;
};
