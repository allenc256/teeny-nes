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
  uint8_t          peek(uint16_t offset) const;

  void step();

  void enable_logging(std::ostream *os) { log_.enable(os); }

private:
  class Log {
  public:
    Log(Cpu &cpu);

    bool enabled() const { return os_; }
    void enable(std::ostream *os) { os_ = os; }
    void begin_step();
    void op_imm16(std::string_view name, uint16_t x);
    void decode(uint8_t x);
    void end_step();

  private:
    std::ostringstream buf_;
    std::ostream      *os_;
    Cpu               &cpu_;
  };

  void power_up();
  void reset();

  uint8_t  decode8();
  uint16_t decode16();

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
