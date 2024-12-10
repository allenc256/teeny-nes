#include <cstring>
#include <format>
#include <iostream>

#include "src/app/app.h"
#include "src/emu/nes.h"

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
  App window;
  window.run();
  return 0;

  // // Create window with SDL_Renderer graphics context
  // SDL_WindowFlags window_flags =
  //     (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  // SDL_Window *window = SDL_CreateWindow(
  //     "teeny-nes",
  //     SDL_WINDOWPOS_CENTERED,
  //     SDL_WINDOWPOS_CENTERED,
  //     1280,
  //     720,
  //     window_flags
  // );
  // if (window == nullptr) {
  //   printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
  //   return -1;
  // }

  // Nes nes;
  // nes.load_cart("hello_world.nes");
  // nes.power_up();

  // Cpu &cpu = nes.cpu();
  // Ppu &ppu = nes.ppu();

  // for (int i = 0; i < 10000; i++) {
  //   std::cout << cpu.disassemble();
  //   std::cout << std::format(
  //       " {:03d},{:03d},{},{}\n",
  //       ppu.scanline(),
  //       ppu.scanline_tick(),
  //       (int)ppu.ready(),
  //       ppu.cycles()
  //   );
  //   nes.step();
  // }

  // // dump_pattern_tables(nes.ppu());
  // // dump_name_table(nes.ppu(), 0x2000);

  // return 0;
}

// int mainint ) {
//   if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
//     std::cerr << std::format("SDL_Init() failed: {}\n", SDL_GetError());
//     return 1;
//   }

//   SDL_Window *window =
//       SDL_CreateWindow("my window", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
//   if (!window) {
//     std::cerr << std::format("SDL_CreateWindow() failed: {}\n",
//     SDL_GetError()); return 1;
//   }

//   SDL_Surface *winSurface = SDL_GetWindowSurface(window);
//   if (!winSurface) {
//     std::cerr << "Error getting surface: " << SDL_GetError() << std::endl;
//     return 1;
//   }

//   // Fill the window with a white rectangle
//   SDL_FillRect(winSurface, NULL, SDL_MapRGB(winSurface->format, 255, 255,
//   255));

//   // Update the window display
//   SDL_UpdateWindowSurface(window);

//   while (true) {
//     SDL_Event e;
//     if (SDL_PollEvent(&e)) {
//       if (e.type == SDL_QUIT) {
//         break;
//       }
//     }
//   }

//   // // Wait
//   // char ch;
//   // std::cin >> ch;

//   SDL_DestroyWindow(window);
//   SDL_Quit();
//   return 0;

//   // Nes nes;
//   // nes.load_cart("hello_world.nes");
//   // nes.power_up();

//   // Cpu &cpu = nes.cpu();
//   // Ppu &ppu = nes.ppu();

//   // while (cpu.registers().PC != 0x80dc) {
//   //   nes.step();
//   // }

//   // // for (int i = 0; i < 500; i++) {
//   // //   std::cout << cpu.disassemble();
//   // //   std::cout << std::format(
//   // //       " {:03d},{:03d},{}\n", ppu.scanline(), ppu.scanline_tick(),
//   // //       ppu.frames()
//   // //   );
//   // //   nes.step();
//   // // }

//   // dump_pattern_table(ppu, 0x0000);
//   // dump_name_table(ppu, 0x2000);

//   // return 0;
// }
