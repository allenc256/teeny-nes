#pragma once

#include <bit>

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
  if (x < max) {
    set_bits<mask>(bits, x + 1);
    return false;
  } else {
    set_bits<mask>(bits, 0);
    return true;
  }
}