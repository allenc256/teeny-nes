#pragma once

#include <cstdint>

inline constexpr int64_t cpu_to_ppu_cycles(int64_t cpu_cycles) {
  return cpu_cycles * 3;
}
