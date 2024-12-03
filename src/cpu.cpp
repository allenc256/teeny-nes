#include "cpu.h"

#include <cstring>
#include <format>
#include <iomanip>
#include <iostream>

static void pad_to_column(std::string &buf, size_t col) {
  while (buf.size() < col) {
    buf.push_back(' ');
  }
}

Cpu::Log::Log(Cpu &cpu) : os_(nullptr), cpu_(cpu) {}

void Cpu::Log::decode(uint8_t x) {
  if (!enabled()) {
    return;
  }
  std::format_to(std::back_inserter(buf_), "{:02X} ", (int)x);
}

void Cpu::Log::begin_step() {
  if (!enabled()) {
    return;
  }
  buf_.clear();
  std::format_to(std::back_inserter(buf_), "{:04X}  ", (int)cpu_.regs_.PC);
}

void Cpu::Log::op_abs(std::string_view op_name, uint16_t x) {
  if (!enabled()) {
    return;
  }
  pad_to_column(buf_, 16);
  std::format_to(std::back_inserter(buf_), "{} ${:04X}", op_name, x);
  finish();
}

void Cpu::Log::op_rel(std::string_view op_name, uint16_t x) {
  if (!enabled()) {
    return;
  }
  pad_to_column(buf_, 16);
  std::format_to(std::back_inserter(buf_), "{} ${:04X}", op_name, x);
  finish();
}

void Cpu::Log::op_imm(std::string_view op_name, uint8_t x) {
  if (!enabled()) {
    return;
  }
  pad_to_column(buf_, 16);
  std::format_to(std::back_inserter(buf_), "{} #${:02X}", op_name, x);
  finish();
}

void Cpu::Log::op_zp(std::string_view op_name, uint8_t x, uint8_t v) {
  if (!enabled()) {
    return;
  }
  pad_to_column(buf_, 16);
  std::format_to(
      std::back_inserter(buf_), "{} ${:02X} = {:02X}", op_name, x, v
  );
  finish();
}

void Cpu::Log::op(std::string_view op_name) {
  if (!enabled()) {
    return;
  }
  pad_to_column(buf_, 16);
  std::format_to(std::back_inserter(buf_), "{}", op_name);
  finish();
}

void Cpu::Log::finish() {
  assert(enabled());
  pad_to_column(buf_, 48);
  std::format_to(
      std::back_inserter(buf_),
      "A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X} PPU:???,??? CYC:{}\n",
      (int)cpu_.regs_.A,
      (int)cpu_.regs_.X,
      (int)cpu_.regs_.Y,
      (int)cpu_.regs_.P,
      (int)cpu_.regs_.S,
      cpu_.cycles_
  );
  *os_ << buf_;
}

Cpu::Cpu(Cartridge &cart) : cycles_(0), cart_(cart), log_(*this) { power_up(); }

void Cpu::power_up() {
  regs_.A  = 0;
  regs_.X  = 0;
  regs_.Y  = 0;
  regs_.PC = (uint16_t)cart_.peek(0xfffc) | (uint16_t)(cart_.peek(0xfffd) << 8);
  regs_.S  = 0xfd;
  regs_.P  = I_FLAG | DUMMY_FLAG;

  std::memset(ram_, 0, sizeof(ram_));
}

void Cpu::reset() {
  regs_.PC = (uint16_t)cart_.peek(0xfffc) | (uint16_t)(cart_.peek(0xfffd) << 8);
  regs_.S -= 3;
  regs_.P |= I_FLAG;
}

uint8_t Cpu::peek(uint16_t addr) const {
  if (addr <= RAM_ADDR_MAX) {
    return ram_[addr & RAM_MASK];
  } else if (addr >= Cartridge::CART_OFFSET) {
    return cart_.peek(addr);
  } else {
    throw std::runtime_error("unsupported memory mapped address");
  }
}

void Cpu::poke(uint16_t addr, uint8_t value) {
  if (addr <= RAM_ADDR_MAX) {
    ram_[addr & RAM_MASK] = value;
  } else if (addr >= Cartridge::CART_OFFSET) {
    cart_.poke(addr, value);
  } else {
    throw std::runtime_error("unsupported memory mapped address");
  }
}

void Cpu::push16(uint16_t x) {
  ram_[STACK_OFFSET + regs_.S--] = x >> 8;
  ram_[STACK_OFFSET + regs_.S--] = x & 0xff;
}

uint16_t Cpu::pop16() {
  uint16_t a = ram_[STACK_OFFSET + (++regs_.S)];
  a |= (uint16_t)ram_[STACK_OFFSET + (++regs_.S)] << 8;
  return a;
}

void Cpu::step() {
  log_.begin_step();

  uint8_t op = decode8();
  switch (op) {
  case 0x4c: step_JMP_abs(); break;
  case 0xa2: step_LDX_imm(); break;
  case 0xa9: step_LDA_imm(); break;
  case 0x86: step_STX_zp(); break;
  case 0x85: step_STA_zp(); break;
  case 0x20: step_JSR_abs(); break;
  case 0x60: step_RTS(); break;
  case 0xea: step_NOP(); break;
  case 0x38: step_SEC(); break;
  case 0x78: step_SEI(); break;
  case 0xf8: step_SED(); break;
  case 0x18: step_CLC(); break;
  case 0xb0: step_BCS_rel(); break;
  case 0x90: step_BCC_rel(); break;
  case 0xf0: step_BEQ_rel(); break;
  case 0xd0: step_BNE_rel(); break;
  case 0x70: step_BVS_rel(); break;
  case 0x50: step_BVC_rel(); break;
  case 0x10: step_BPL_rel(); break;
  case 0x24: step_BIT_zp(); break;
  default:
    throw std::runtime_error(std::format("unimplemented opcode: ${:02X}", op));
  }
}

uint8_t Cpu::decode8() {
  uint8_t x = peek(regs_.PC++);
  log_.decode(x);
  return x;
}

uint16_t Cpu::decode16() {
  return (uint16_t)decode8() | (uint16_t)(decode8() << 8);
}

void Cpu::update_Z_flag(uint8_t x) {
  if (x == 0) {
    regs_.P |= Z_FLAG;
  } else {
    regs_.P &= ~Z_FLAG;
  }
}

void Cpu::update_N_flag(uint8_t x) {
  if (x & 0b10000000) {
    regs_.P |= N_FLAG;
  } else {
    regs_.P &= ~N_FLAG;
  }
}

void Cpu::step_JMP_abs() {
  uint16_t x = decode16();
  log_.op_abs("JMP", x);
  regs_.PC = x;
  cycles_ += 3;
}

void Cpu::step_LDX_imm() {
  uint8_t x = decode8();
  log_.op_imm("LDX", x);
  regs_.X = x;
  update_Z_flag(x);
  update_N_flag(x);
  cycles_ += 2;
}

void Cpu::step_LDA_imm() {
  uint8_t x = decode8();
  log_.op_imm("LDA", x);
  regs_.A = x;
  update_Z_flag(x);
  update_N_flag(x);
  cycles_ += 2;
}

void Cpu::step_STX_zp() {
  uint8_t x = decode8();
  log_.op_zp("STX", x, peek(x));
  poke(x, regs_.X);
  cycles_ += 3;
}

void Cpu::step_STA_zp() {
  uint8_t x = decode8();
  log_.op_zp("STA", x, peek(x));
  poke(x, regs_.A);
  cycles_ += 3;
}

void Cpu::step_JSR_abs() {
  uint16_t x = decode16();
  log_.op_abs("JSR", x);
  push16(regs_.PC - 1);
  regs_.PC = x;
  cycles_ += 6;
}

void Cpu::step_RTS() {
  log_.op("RTS");
  regs_.PC = pop16() + 1;
  cycles_ += 6;
}

void Cpu::step_NOP() {
  log_.op("NOP");
  cycles_ += 2;
}

void Cpu::step_SEC() {
  log_.op("SEC");
  regs_.P |= C_FLAG;
  cycles_ += 2;
}

void Cpu::step_CLC() {
  log_.op("CLC");
  regs_.P &= ~C_FLAG;
  cycles_ += 2;
}

static bool page_crossed(uint16_t old_val, uint16_t new_val) {
  return (old_val & 0xff00) != (new_val & 0xff00);
}

void Cpu::step_BCS_rel() {
  step_BXX_rel("BCS", [&]() { return regs_.P & C_FLAG; });
}

void Cpu::step_BCC_rel() {
  step_BXX_rel("BCC", [&]() { return !(regs_.P & C_FLAG); });
}

void Cpu::step_BEQ_rel() {
  step_BXX_rel("BEQ", [&]() { return regs_.P & Z_FLAG; });
}

void Cpu::step_BNE_rel() {
  step_BXX_rel("BNE", [&]() { return !(regs_.P & Z_FLAG); });
}

void Cpu::step_BVS_rel() {
  step_BXX_rel("BVS", [&]() { return regs_.P & V_FLAG; });
}

void Cpu::step_BVC_rel() {
  step_BXX_rel("BVC", [&]() { return !(regs_.P & V_FLAG); });
}

void Cpu::step_BPL_rel() {
  step_BXX_rel("BPL", [&]() { return !(regs_.P & N_FLAG); });
}

void Cpu::step_BIT_zp() {
  uint8_t a = decode8();
  uint8_t m = peek(a);
  log_.op_zp("BIT", a, m);
  update_Z_flag(regs_.A & m);
  update_N_flag(m);
  if (m & 0b01000000) {
    regs_.P |= V_FLAG;
  } else {
    regs_.P &= ~V_FLAG;
  }
  cycles_ += 3;
}

template <typename Test>
void Cpu::step_BXX_rel(std::string_view op_name, Test test) {
  int8_t   x = (int8_t)decode8();
  uint16_t a = (uint16_t)(regs_.PC + x);
  log_.op_rel(op_name, a);
  if (test()) {
    if (page_crossed(regs_.PC, a)) {
      cycles_ += 4;
    } else {
      cycles_ += 3;
    }
    regs_.PC = a;
  } else {
    cycles_ += 2;
  }
}

void Cpu::step_SEI() {
  log_.op("SEI");
  regs_.P |= I_FLAG;
  cycles_ += 2;
}

void Cpu::step_SED() {
  log_.op("SED");
  regs_.P |= D_FLAG;
  cycles_ += 2;
}
