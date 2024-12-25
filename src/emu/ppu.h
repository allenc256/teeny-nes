#pragma once

#include <cstdint>
#include <memory>

#include "src/emu/coroutine.h"

class Cpu;
class Cart;

class SpriteBuf {
public:
  SpriteBuf();

  void clear();
  void render(int x, int pattern, int palette, bool &behind, bool &spr0);
  void get(int x, int &pattern, int &palette, bool &behind, bool &spr0) const;

private:
  uint8_t bytes_[256];
};

class Ppu {
public:
  struct Registers {
    uint8_t PPUCTRL;
    uint8_t PPUMASK;
    uint8_t PPUSTATUS;
    uint8_t OAMADDR;
    uint8_t PPUDATA;
    uint8_t OAMDMA;

    uint16_t v;
    uint16_t t;
    uint8_t  x;
    uint8_t  w;

    uint16_t shift_bg_lo;
    uint16_t shift_bg_hi;
    uint16_t shift_at_lo;
    uint16_t shift_at_hi;
  };

  Ppu();

  void set_cpu(Cpu *cpu) { cpu_ = cpu; }
  void set_ready(bool ready) { ready_ = ready; }

  void set_cart(Cart *cart) {
    cart_      = cart;
    step_cart_ = true;
  }

  Registers     &registers() { return regs_; }
  int            scanline() const { return scanline_; }
  int            dot() const { return dot_; }
  int64_t        cycles() const { return cycles_; }
  int64_t        frames() const { return frames_; }
  bool           ready() const { return ready_; }
  const uint8_t *frame() const { return front_frame_.get(); }
  uint16_t       addr_bus() const { return addr_bus_; }

  bool     rendering() const;
  bool     bg_rendering() const;
  bool     spr_rendering() const;
  bool     bg_show_left() const;
  bool     spr_show_left() const;
  uint16_t bg_pt_base_addr() const;
  uint16_t spr_pt_base_addr() const;
  int      color_emphasis() const;

  uint8_t read_PPUCTRL();
  uint8_t read_PPUMASK();
  uint8_t read_PPUSTATUS();
  uint8_t read_OAMADDR();
  uint8_t read_OAMDATA();
  uint8_t read_OAMDMA();
  uint8_t read_PPUSCROLL();
  uint8_t read_PPUADDR();
  uint8_t read_PPUDATA();

  void write_PPUCTRL(uint8_t x);
  void write_PPUMASK(uint8_t x);
  void write_PPUSTATUS(uint8_t x);
  void write_OAMADDR(uint8_t x);
  void write_OAMDATA(uint8_t x);
  void write_OAMDMA(uint8_t x);
  void write_PPUSCROLL(uint8_t x);
  void write_PPUADDR(uint8_t x);
  void write_PPUDATA(uint8_t x);

  void    poke(uint16_t addr, uint8_t x);
  uint8_t peek(uint16_t addr);

  void power_on();
  void reset();
  void step();

private:
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
  static constexpr uint8_t PPUCTRL_SPR_ADDR   = 0b00001000;
  static constexpr uint8_t PPUCTRL_BG_ADDR    = 0b00010000;
  static constexpr uint8_t PPUCTRL_SPR_SIZE   = 0b00100000;
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
  static constexpr uint8_t PPUMASK_GRAY          = 0b00000001;
  static constexpr uint8_t PPUMASK_BG_SHOW_LEFT  = 0b00000010;
  static constexpr uint8_t PPUMASK_SPR_SHOW_LEFT = 0b00000100;
  static constexpr uint8_t PPUMASK_BG_RENDERING  = 0b00001000;
  static constexpr uint8_t PPUMASK_SPR_RENDERING = 0b00010000;
  static constexpr uint8_t PPUMASK_RENDERING     = 0b00011000;
  static constexpr uint8_t PPUMASK_EMPHASIS      = 0b11100000;

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
  static constexpr uint8_t PPUSTATUS_SPR_OVF  = 0b00100000;
  static constexpr uint8_t PPUSTATUS_SPR0_HIT = 0b01000000;
  static constexpr uint8_t PPUSTATUS_VBLANK   = 0b10000000;
  static constexpr uint8_t PPUSTATUS_ALL      = 0b11100000;

  static constexpr uint16_t V_COARSE_X     = 0b00000000'00011111;
  static constexpr uint16_t V_COARSE_Y     = 0b00000011'11100000;
  static constexpr uint16_t V_NAME_TABLE   = 0b00001100'00000000;
  static constexpr uint16_t V_NAME_TABLE_H = 0b00000100'00000000;
  static constexpr uint16_t V_NAME_TABLE_V = 0b00001000'00000000;
  static constexpr uint16_t V_FINE_Y       = 0b01110000'00000000;
  static constexpr uint16_t V_HI           = 0b01111111'00000000;
  static constexpr uint16_t V_LO           = 0b00000000'11111111;
  static constexpr int      V_COARSE_X_MAX = 31;
  static constexpr int      V_COARSE_Y_MAX = 29;
  static constexpr int      V_FINE_Y_MAX   = 7;

  // Sprite attributes
  // =================
  //
  // 76543210
  // ||||||||
  // ||||||++- Palette (4 to 7) of sprite
  // |||+++--- Unimplemented (read 0)
  // ||+------ Priority (0: in front of background; 1: behind background)
  // |+------- Flip sprite horizontally
  // +-------- Flip sprite vertically
  static constexpr uint8_t SPR_ATTR_PALETTE   = 0b00000011;
  static constexpr uint8_t SPR_ATTR_PRIO      = 0b00100000;
  static constexpr uint8_t SPR_ATTR_FLIP_HORZ = 0b01000000;
  static constexpr uint8_t SPR_ATTR_FLIP_VERT = 0b10000000;

  static constexpr int VISIBLE_FRAME_END   = 240;
  static constexpr int PRE_RENDER_SCANLINE = 261;

  void step_visible_frame();
  void step_pre_render_scanline();
  void step_post_render_scanline();
  void next_dot();

  uint8_t read_open_bus();
  void    draw_dot();

  Coroutine bg_loop();
  uint8_t   bg_loop_fetch_nt();
  uint8_t   bg_loop_fetch_at();
  uint8_t   bg_loop_fetch_pt_lo(uint8_t nt);
  uint8_t   bg_loop_fetch_pt_hi(uint8_t nt);
  void      bg_loop_shift_regs();
  void      bg_loop_reload_regs(uint8_t at, uint8_t pt_lo, uint8_t pt_hi);
  void      bg_loop_inc_v_horz();
  void      bg_loop_inc_v_vert();
  void      bg_loop_set_v_horz();
  void      bg_loop_set_v_vert();

  Coroutine spr_loop();
  void
  spr_loop_render(int x, uint8_t attr, uint8_t pt_lo, uint8_t pt_hi, bool spr0);

  using Frame = std::unique_ptr<uint8_t[]>;

  Coroutine bg_loop_;
  Coroutine spr_loop_;

  Registers regs_;
  uint8_t   vram_[2 * 1024];
  uint8_t   palette_[32];
  uint8_t   oam_[256];
  uint8_t   soam_[32];
  uint16_t  addr_bus_;
  SpriteBuf spr_buf_;
  Cart     *cart_;
  Cpu      *cpu_;
  int       scanline_;
  int       dot_;
  Frame     back_frame_;
  Frame     front_frame_;
  int64_t   cycles_; // since reset
  int64_t   frames_; // since reset
  bool      step_cart_;

  // Readiness for writes. This is a separate flag rather than just checking the
  // cycle or frame count so that we can force it to true in tests.
  bool ready_;
};
