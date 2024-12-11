#pragma once

#include <cstdint>

class Pixel {
public:
  constexpr Pixel(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
      : bits_((uint32_t)(b | (g << 8) | (r << 16) | (a << 24))) {}

  constexpr Pixel(uint8_t r, uint8_t g, uint8_t b)
      : bits_((uint32_t)(b | (g << 8) | (r << 16))) {}

  uint8_t a() const { return bits_ >> 24; }
  uint8_t r() const { return (bits_ >> 16) & 0xff; }
  uint8_t g() const { return (bits_ >> 8) & 0xff; }
  uint8_t b() const { return bits_ & 0xff; }

private:
  uint32_t bits_;
};
