#pragma once

#include <cstdint>
#include <memory>

class Cpu;
class Cart;

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

    uint8_t fetch_nt;
    uint8_t fetch_at;
    uint8_t fetch_bg_lo;
    uint8_t fetch_bg_hi;
  };

  // N.B., ops are ordered in precedence (the first listed op is executed
  // first, and the last listed op is executed last).
  enum Ops {
    OP_DRAW,
    OP_S_REG_SHIFT_BG,
    OP_S_REG_SHIFT_SP,
    OP_S_REG_RELOAD_BG,
    OP_FETCH_NT,
    OP_FETCH_AT,
    OP_FETCH_BG_LO,
    OP_FETCH_BG_HI,
    OP_FLAG_SET_VBLANK,
    OP_FLAG_CLEAR,
    OP_V_REG_INC_HORZ,
    OP_V_REG_INC_VERT,
    OP_V_REG_SET_HORZ,
    OP_V_REG_SET_VERT,
  };

  Ppu();

  void set_cpu(Cpu *cpu) { cpu_ = cpu; }
  void set_cart(Cart *cart) { cart_ = cart; }
  void set_ready(bool ready) { ready_ = ready; }

  Registers     &registers() { return regs_; }
  int            scanline() const { return scanline_; }
  int            dot() const { return dot_; }
  int64_t        cycles() const { return cycles_; }
  int64_t        frames() const { return frames_; }
  bool           ready() const { return ready_; }
  const uint8_t *frame() const { return front_frame_.get(); }

  uint16_t bg_pattern_table_addr() const;
  bool     bg_rendering() const;
  int      emphasis() const;

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
  static constexpr uint8_t PPUMASK_EMPHASIS     = 0b11100000;

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

  static constexpr int VISIBLE_FRAME_END   = 240;
  static constexpr int PRE_RENDER_SCANLINE = 261;

  void next_dot();
  void execute_ops(uint16_t op_mask);

  uint8_t read_open_bus();

  void op_fetch_nt();
  void op_fetch_at();
  void op_fetch_bg_lo();
  void op_fetch_bg_hi();
  void op_flag_set_vblank();
  void op_flag_clear();
  void op_v_reg_inc_horz();
  void op_v_reg_inc_vert();
  void op_v_reg_set_horz();
  void op_v_reg_set_vert();
  void op_s_reg_shift_bg();
  void op_s_reg_shift_sp();
  void op_s_reg_reload_bg();
  void op_draw();

  using Frame = std::unique_ptr<uint8_t[]>;

  Registers regs_;
  uint8_t   vram_[2 * 1024];
  uint8_t   palette_[32];
  uint8_t   oam_[256];
  Cart     *cart_;
  Cpu      *cpu_;
  int       scanline_;
  int       dot_;
  Frame     back_frame_;
  Frame     front_frame_;
  int64_t   cycles_; // since reset
  int64_t   frames_; // since reset

  // Readiness for writes. This is a separate flag rather than just checking the
  // cycle or frame count so that we can force it to true in tests.
  bool ready_;
};
