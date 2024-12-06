#include "nes.h"

#include <cstring>
#include <format>
#include <iostream>

void extract_pattern(Ppu &ppu, uint16_t addr, uint8_t (*buf)[8]) {
  std::memset(buf, 0, 64);

  for (uint16_t i = 0; i < 16; i++) {
    uint8_t mem = ppu.peek(addr + i);
    for (int j = 0; j < 8; j++) {
      if (mem & (1u << (8 - j))) {
        buf[i & 7][j] += 1;
      }
    }
  }
}

void extract_pattern_table(Ppu &ppu, uint16_t base, uint8_t (*buf)[16][8][8]) {
  for (int i = 0; i < 256; i++) {
    uint16_t off  = (uint16_t)(i << 4);
    uint16_t addr = base + off;
    int      row  = i >> 4;
    int      col  = i & 0xf;
    extract_pattern(ppu, addr, buf[row][col]);
  }
}

void dump_pattern_table(Ppu &ppu, uint16_t base) {
  static constexpr char char_map[4] = {'.', '1', '2', '3'};
  uint8_t               buf[16][16][8][8];

  extract_pattern_table(ppu, base, buf);

  for (int row = 0; row < 16; row++) {
    for (int y = 0; y < 8; y++) {
      for (int col = 0; col < 16; col++) {
        for (int x = 0; x < 8; x++) {
          std::cout << char_map[buf[row][col][y][x]];
        }
      }
      std::cout << '\n';
    }
  }
}

void dump_pattern_tables(Ppu &ppu) {
  std::cout << "------------------\n";
  std::cout << "Pattern table 0000\n";
  std::cout << "------------------\n";
  std::cout << '\n';
  dump_pattern_table(ppu, 0x0000);
  std::cout << '\n';
  std::cout << "------------------\n";
  std::cout << "Pattern table 1000\n";
  std::cout << "------------------\n";
  std::cout << '\n';
  dump_pattern_table(ppu, 0x1000);
}

int main() {
  Nes nes;
  nes.load_cart("hello_world.nes");
  nes.power_up();

  dump_pattern_tables(nes.ppu());

  return 0;
}
