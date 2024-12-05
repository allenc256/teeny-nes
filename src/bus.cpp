#include "bus.h"

#include <format>
#include <stdexcept>

uint8_t Bus::peek(uint16_t addr) {
  if (addr < RAM_END) {
    return ram_[addr & RAM_MASK];
  } else if (addr >= Apu::CHAN_START && addr < Apu::CHAN_END) {
    // TODO: implement "open bus behavior"
    // (https://www.nesdev.org/wiki/Open_bus_behavior)
    return 0xff;
  } else if (addr == Apu::CONT_ADDR) {
    return apu_.peek_control();
  } else if (addr >= Cartridge::ADDR_START) {
    return cart_.peek(addr);
  } else {
    throw std::runtime_error(
        std::format("unsupported address: peek(${:04X})", addr)
    );
  }
}

void Bus::poke(uint16_t addr, uint8_t x) {
  if (addr < RAM_END) {
    ram_[addr & RAM_MASK] = x;
  } else if (addr >= Apu::CHAN_START && addr < Apu::CHAN_END) {
    apu_.poke_channel(addr, x);
  } else if (addr == Apu::CONT_ADDR) {
    apu_.poke_control(x);
  } else if (addr >= Cartridge::ADDR_START) {
    cart_.poke(addr, x);
  } else {
    throw std::runtime_error(
        std::format("unsupported address: poke(${:04X}, {:02X})", addr, x)
    );
  }
}

uint8_t TestBus::peek(uint16_t addr) { return ram_[addr]; }
void    TestBus::poke(uint16_t addr, uint8_t x) { ram_[addr] = x; }
