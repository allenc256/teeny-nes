#pragma once

#include <bit>
#include <cstdint>

template <uint16_t mask> constexpr uint16_t get_bits(uint16_t x) {
  constexpr int off = std::countr_zero(mask);
  return (x & mask) >> off;
}

template <uint16_t mask> constexpr void set_bits(uint16_t &bits, int x) {
  constexpr int off = std::countr_zero(mask);
  bits              = (bits & ~mask) | ((x << off) & mask);
}

template <uint16_t mask> constexpr void copy_bits(uint16_t from, uint16_t &to) {
  to = (to & ~mask) | (from & mask);
}

template <uint16_t mask, int max> constexpr bool inc_bits(uint16_t &bits) {
  int x = get_bits<mask>(bits);
  // N.B., values above "max" are allowed to wrap around without reporting
  // overflow intentionally. This allows us to specifically emulate behavior on
  // the PPU where incrementing the "coarse_y" component of the PPU "v" register
  // should only trigger certain overflow conditions when "coarse_y" == 29 (even
  // though it's technically possible for it to take higher values).
  if (x != max) {
    set_bits<mask>(bits, x + 1);
    return false;
  } else {
    set_bits<mask>(bits, 0);
    return true;
  }
}
