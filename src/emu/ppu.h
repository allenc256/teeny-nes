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
  void set_cart(Cart *cart) { cart_ = cart; }

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

  // Readiness for writes. This is a separate flag rather than just checking the
  // cycle or frame count so that we can force it to true in tests.
  bool ready_;
};

inline constexpr int64_t cpu_to_ppu_cycles(int64_t cpu_cycles) {
  return cpu_cycles * 3;
}
