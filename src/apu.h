#pragma once

#include <cassert>
#include <cstdint>
#include <stdexcept>

class Apu {
public:
  static constexpr uint16_t CHAN_START = 0x4000;
  static constexpr uint16_t CHAN_END   = 0x4010;
  static constexpr uint16_t CONT_ADDR  = 0x4015;

  void power_up() {
    // no-op
  }

  void reset() {
    // no-op
  }

  uint8_t peek_control() {
    // TODO: implement me
    return 0xff;
  }

  void poke_control([[maybe_unused]] uint8_t x) {
    // no-op for now
  }

  void
  poke_channel([[maybe_unused]] uint16_t addr, [[maybe_unused]] uint8_t x) {
    assert(addr >= CHAN_START && addr < CHAN_END);
    // no-op for now
  }
};
