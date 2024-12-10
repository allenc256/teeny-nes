#pragma once

#include "emu/cycles.h"

#include <cstdint>

class Cpu;
class Cart;

class Ppu {
public:
  struct Registers {
    uint8_t  PPUCTRL;
    uint8_t  PPUMASK;
    uint8_t  PPUSTATUS;
    uint8_t  OAMADDR;
    uint8_t  PPUDATA;
    uint8_t  OAMDMA;
    uint16_t v;
    uint16_t t;
    uint8_t  x;
    uint8_t  w;
  };

  enum FetchOp : uint8_t {
    FETCH_OP_NONE = 0,
    FETCH_OP_NT_BYTE_CYC1,
    FETCH_OP_NT_BYTE_CYC2,
    FETCH_OP_AT_BYTE_CYC1,
    FETCH_OP_AT_BYTE_CYC2,
    FETCH_OP_PT_BYTE_LO_CYC1,
    FETCH_OP_PT_BYTE_LO_CYC2,
    FETCH_OP_PT_BYTE_HI_CYC1,
    FETCH_OP_PT_BYTE_HI_CYC2,
  };

  enum VOp : uint8_t {
    V_OP_NONE = 0,
    V_OP_INC_HORZ,
    V_OP_INC_VERT,
    V_OP_SET_HORZ,
    V_OP_SET_VERT,
  };

  struct CycleOps {
    FetchOp fetch_op : 4 = FETCH_OP_NONE;
    VOp     v_op : 4     = V_OP_NONE;
  };

  Ppu();

  void set_cpu(Cpu *cpu) { cpu_ = cpu; }
  void set_cart(Cart *cart) { cart_ = cart; }
  void set_ready(bool ready) { ready_ = ready; }

  Registers &registers() { return regs_; }
  int        scanline() const { return scanline_; }
  int        scanline_tick() const { return scanline_cycle_; }
  int64_t    cycles() const { return cycles_; }
  int64_t    frames() const { return frames_; }
  bool       ready() const { return ready_; }

  uint8_t read_PPUCTRL();
  uint8_t read_PPUMASK();
  uint8_t read_PPUSTATUS();
  uint8_t read_OAMADDR();
  uint8_t read_OAMDATA();
  uint8_t read_PPUSCROLL();
  uint8_t read_PPUADDR();
  uint8_t read_PPUDATA();
  uint8_t read_OAMDMA();

  void write_PPUCTRL(uint8_t x);
  void write_PPUMASK(uint8_t x);
  void write_PPUSTATUS(uint8_t x);
  void write_OAMADDR(uint8_t x);
  void write_OAMDATA(uint8_t x);
  void write_PPUSCROLL(uint8_t x);
  void write_PPUADDR(uint8_t x);
  void write_PPUDATA(uint8_t x);
  void write_OAMDMA(uint8_t x);

  void    poke(uint16_t addr, uint8_t x);
  uint8_t peek(uint16_t addr);

  void power_up();
  void reset();
  void step();

private:
  uint8_t read_open_bus();

  // PPUCTRL layout
  // ==============
  //
  // 7  bit  0
  // ---- ----
  // VPHB SINN
  // |||| ||||
  // |||| ||++- Base nametable address
  // |||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
  // |||| |+--- VRAM address increment per CPU read/write of PPUDATA
  // |||| |     (0: add 1, going across; 1: add 32, going down)
  // |||| +---- Sprite pattern table address for 8x8 sprites
  // ||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
  // |||+------ Background pattern table address (0: $0000; 1: $1000)
  // ||+------- Sprite size (0: 8x8 pixels; 1: 8x16 pixels – see PPU OAM#Byte 1)
  // |+-------- PPU master/slave select
  // |          (0: read backdrop from EXT pins; 1: output color on EXT pins)
  // +--------- Vblank NMI enable (0: off, 1: on)
  static constexpr uint8_t PPUCTRL_NN_ADDR    = 0b00000011;
  static constexpr uint8_t PPUCTRL_VRAM_INC   = 0b00000100;
  static constexpr uint8_t PPUCTRL_SP_ADDR    = 0b00001000;
  static constexpr uint8_t PPUCTRL_BG_ADDR    = 0b00010000;
  static constexpr uint8_t PPUCTRL_SP_SIZE    = 0b00100000;
  static constexpr uint8_t PPUCTRL_MSTR_SLAVE = 0b01000000;
  static constexpr uint8_t PPUCTRL_NMI_ENABLE = 0b10000000;

  // PPUMASK layout
  // ==============
  //
  //   7  bit  0
  // ---- ----
  // BGRs bMmG
  // |||| ||||
  // |||| |||+- Greyscale (0: normal color, 1: greyscale)
  // |||| ||+-- 1: Show background in leftmost 8 pixels of screen, 0: Hide
  // |||| |+--- 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
  // |||| +---- 1: Enable background rendering
  // |||+------ 1: Enable sprite rendering
  // ||+------- Emphasize red (green on PAL/Dendy)
  // |+-------- Emphasize green (red on PAL/Dendy)
  // +--------- Emphasize blue
  static constexpr uint8_t PPUMASK_GRAY         = 0b00000001;
  static constexpr uint8_t PPUMASK_BG_LEFT      = 0b00000010;
  static constexpr uint8_t PPUMASK_SP_LEFT      = 0b00000100;
  static constexpr uint8_t PPUMASK_BG_RENDERING = 0b00001000;
  static constexpr uint8_t PPUMASK_SP_RENDERING = 0b00010000;
  static constexpr uint8_t PPUMASK_RENDERING    = 0b00011000;
  static constexpr uint8_t PPUMASK_RED          = 0b00100000;
  static constexpr uint8_t PPUMASK_GREEN        = 0b01000000;
  static constexpr uint8_t PPUMASK_BLUE         = 0b10000000;

  // PPUSTATUS layout
  // ================
  //
  //   7  bit  0
  // ---- ----
  // VSOx xxxx
  // |||| ||||
  // |||+-++++- (PPU open bus or 2C05 PPU identifier)
  // ||+------- Sprite overflow flag
  // |+-------- Sprite 0 hit flag
  // +--------- Vblank flag, cleared on read. Unreliable; see below.
  static constexpr uint8_t PPUSTATUS_SP_OVF  = 0b00100000;
  static constexpr uint8_t PPUSTATUS_SP_HIT0 = 0b01000000;
  static constexpr uint8_t PPUSTATUS_VBLANK  = 0b10000000;
  static constexpr uint8_t PPUSTATUS_ALL     = 0b11100000;

  static constexpr int VISIBLE_FRAME_END   = 240;
  static constexpr int PRE_RENDER_SCANLINE = 261;

  static_assert(sizeof(Ppu::CycleOps) == 1);

  void step_pre_render();
  void step_visible_frame();
  void step_post_render();
  void step_internal(CycleOps ops);

  void set_vblank();
  void clear_flags();
  void fetch_NT_byte_cyc1();
  void fetch_NT_byte_cyc2();
  void fetch_AT_byte_cyc1();
  void fetch_AT_byte_cyc2();
  void fetch_PT_byte_lo_cyc1();
  void fetch_PT_byte_lo_cyc2();
  void fetch_PT_byte_hi_cyc1();
  void fetch_PT_byte_hi_cyc2();
  void inc_v_horizontal();
  void inc_v_vertical();
  void set_v_horizontal();
  void set_v_vertical();
  void draw_pixel();

  Registers regs_;
  uint8_t   vram_[2 * 1024];
  uint8_t   palette_[32];
  uint8_t   oam_[256];
  Cart     *cart_;
  Cpu      *cpu_;
  int       scanline_;
  int       scanline_cycle_;
  int64_t   cycles_; // since reset
  int64_t   frames_; // since reset

  // Readiness for writes. This is a separate flag rather than just checking the
  // cycle or frame count so that we can force it to true in tests.
  bool ready_;
};
