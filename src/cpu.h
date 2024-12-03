#pragma once

#include "cartridge.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>

class Cpu {
public:
  enum PFlag : uint8_t {
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

  Cpu(Cartridge &cart);

  const Registers &registers() const { return regs_; }
  Registers       &registers() { return regs_; }
  int64_t          cycles() const { return cycles_; }
  void             set_cycles(int64_t cycles) { cycles_ = cycles; }

  uint8_t  peek(uint16_t addr) const;
  void     poke(uint16_t addr, uint8_t value);
  void     push16(uint16_t x);
  uint16_t pop16();
  void     step();

  void enable_logging(std::ostream *os) { log_.enable(os); }

private:
  class Log {
  public:
    Log(Cpu &cpu);

    bool enabled() const { return os_; }
    void enable(std::ostream *os) { os_ = os; }
    void begin_step();
    void decode(uint8_t x);
    void op(std::string_view op_name);
    void op_abs(std::string_view op_name, uint16_t x);
    void op_imm(std::string_view op_name, uint8_t x);
    void op_zp(std::string_view op_name, uint8_t x, uint8_t v);
    void op_rel(std::string_view op_name, uint16_t x);

  private:
    void finish();

    std::ostream *os_;
    std::string   buf_;
    Cpu          &cpu_;
  };

  void power_up();
  void reset();

  uint8_t  decode8();
  uint16_t decode16();

  void step_JMP_abs();
  void step_LDX_imm();
  void step_LDA_imm();
  void step_STX_zp();
  void step_JSR_abs();
  void step_RTS();
  void step_NOP();
  void step_SEC();
  void step_SEI();
  void step_SED();
  void step_CLC();
  void step_BCS_rel();
  void step_BCC_rel();
  void step_BEQ_rel();
  void step_BNE_rel();
  void step_BVS_rel();
  void step_BVC_rel();
  void step_BPL_rel();
  void step_STA_zp();
  void step_BIT_zp();

  template <typename Test>
  void step_BXX_rel(std::string_view op_name, Test test);

  void update_Z_flag(uint8_t x);
  void update_N_flag(uint8_t x);

  static constexpr int STACK_OFFSET = 0x0100;
  static constexpr int RAM_SIZE     = 0x0800;
  static constexpr int RAM_MASK     = 0x07ff;
  static constexpr int RAM_ADDR_MAX = 0x1fff;

  uint8_t    ram_[RAM_SIZE];
  uint64_t   cycles_;
  Registers  regs_;
  Cartridge &cart_;
  Log        log_;
};
