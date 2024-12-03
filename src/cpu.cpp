#include "cpu.h"

#include <cstring>
#include <format>
#include <iomanip>
#include <iostream>

static void pad_to_column(std::ostringstream &os, int col) {
  int pos = (int)os.tellp();
  while (pos < col) {
    os << ' ';
    pos++;
  }
}

Cpu::Log::Log(Cpu &cpu) : os_(nullptr), cpu_(cpu) {
  buf_ << std::hex;
  buf_ << std::uppercase;
  buf_ << std::setfill('0');
}

void Cpu::Log::decode(uint8_t x) {
  if (!enabled()) {
    return;
  }
  buf_ << std::setw(2) << x << ' ';
}

void Cpu::Log::begin_step() {
  if (!enabled()) {
    return;
  }
  buf_.clear();
  buf_.seekp(0);
  buf_ << std::setw(4) << cpu_.regs_.PC << "  ";
}

void Cpu::Log::op_imm16(std::string_view name, uint16_t x) {
  if (!enabled()) {
    return;
  }
  pad_to_column(buf_, 16);
  buf_ << name << " $" << std::setw(4) << x;
}

void Cpu::Log::end_step() {
  if (!enabled()) {
    return;
  }
  pad_to_column(buf_, 48);
  buf_ << "A:" << std::setw(2) << cpu_.regs_.A;
  buf_ << " X:" << std::setw(2) << cpu_.regs_.X;
  buf_ << " Y:" << std::setw(2) << cpu_.regs_.Y;
  buf_ << " P:" << std::setw(2) << cpu_.regs_.P;
  buf_ << " SP:" << std::setw(2) << cpu_.regs_.S;
  pad_to_column(buf_, 86);
  buf_ << " CYC:" << std::dec << cpu_.cycles_ << std::hex << '\n';
  *os_ << buf_.str();
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

uint8_t Cpu::peek(uint16_t offset) const {
  if (offset <= RAM_ADDR_MAX) {
    return ram_[offset & RAM_MASK];
  } else if (offset >= Cartridge::CART_OFFSET) {
    return cart_.peek(offset);
  } else {
    throw std::runtime_error("unsupported memory mapped address");
  }
}

void Cpu::step() {
  log_.begin_step();

  uint8_t op = decode8();
  switch (op) {
  case 0x4c: // JMP a
    regs_.PC = decode16();
    cycles_ += 3;
    if (log_.enabled()) {
      log_.op_imm16("JMP", regs_.PC);
    }
  default:
    throw std::runtime_error(std::format("unimplemented opcode: {:02x}", op));
  }

  log_.end_step();
}

uint8_t Cpu::decode8() {
  uint8_t x = peek(regs_.PC++);
  log_.decode(x);
  return x;
}

uint16_t Cpu::decode16() {
  return (uint16_t)decode8() | (uint16_t)(decode8() << 8);
}
