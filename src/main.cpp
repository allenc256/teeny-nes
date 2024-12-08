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

  std::cout << "------------------\n";
  std::cout << std::format("Pattern table {:04X}\n", base);
  std::cout << "------------------\n";
  std::cout << '\n';

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

  std::cout << '\n';
}

void extract_name_table(Ppu &ppu, uint16_t base, uint8_t (*buf)[32]) {
  for (uint16_t i = 0; i < 1024; i++) {
    uint16_t addr = base + i;
    int      x    = i & 0b11111;
    int      y    = i >> 5;
    buf[y][x]     = ppu.peek(addr);
  }
}

void dump_name_table(Ppu &ppu, uint16_t base) {
  uint8_t buf[32][32];

  extract_name_table(ppu, base, buf);

  std::cout << "---------------\n";
  std::cout << std::format("Name table {:04X}\n", base);
  std::cout << "---------------\n";
  std::cout << '\n';

  for (int row = 0; row < 32; row++) {
    for (int col = 0; col < 32; col++) {
      std::cout << std::format("{:02X}", buf[row][col]);
    }
    std::cout << '\n';
  }

  std::cout << '\n';
}

int main() {
  Nes nes;
  nes.load_cart("hello_world.nes");
  nes.power_up();

  Cpu &cpu = nes.cpu();
  Ppu &ppu = nes.ppu();

  for (int i = 0; i < 10000; i++) {
    std::cout << cpu.disassemble();
    std::cout << std::format(
        " {:03d},{:03d},{},{}\n",
        ppu.scanline(),
        ppu.scanline_tick(),
        (int)ppu.odd_frame(),
        (int)ppu.ready()
    );
    nes.step();
  }

  // dump_pattern_tables(nes.ppu());
  // dump_name_table(nes.ppu(), 0x2000);

  return 0;
}
