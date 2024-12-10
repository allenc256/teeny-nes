#pragma once

#include <cstdint>

class Pixel {
public:
  constexpr Pixel(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
      : bits_((uint32_t)(b | (g << 8) | (r << 16) | (a << 24))) {}

  constexpr Pixel(uint8_t r, uint8_t g, uint8_t b)
      : bits_((uint32_t)(b | (g << 8) | (r << 16))) {}

private:
  [[maybe_unused]] uint32_t bits_;
};
