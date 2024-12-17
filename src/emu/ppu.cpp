#include <cassert>
#include <cstring>
#include <format>

#include "src/emu/bits.h"
#include "src/emu/cart.h"
#include "src/emu/cpu.h"
#include "src/emu/ppu.h"

static constexpr int SCANLINE_MAX_CYCLES = 341;

Ppu::Ppu()
    : bg_loop_(bg_loop()),
      cart_(nullptr),
      cpu_(nullptr),
      scanline_(0),
      dot_(0),
      back_frame_(std::make_unique<uint8_t[]>(256 * 240)),
      front_frame_(std::make_unique<uint8_t[]>(256 * 240)),
      cycles_(0),
      frames_(0),
      ready_(false) {}

uint16_t Ppu::bg_pattern_table_addr() const {
  return (uint16_t)((regs_.PPUCTRL & PPUCTRL_BG_ADDR) << 8);
}

bool Ppu::bg_rendering() const { return regs_.PPUMASK & PPUMASK_BG_RENDERING; }
int  Ppu::emphasis() const { return get_bits<PPUMASK_EMPHASIS>(regs_.PPUMASK); }

void Ppu::power_up() {
  regs_.PPUCTRL     = 0;
  regs_.PPUMASK     = 0;
  regs_.PPUSTATUS   = 0b10100000;
  regs_.OAMADDR     = 0;
  regs_.PPUDATA     = 0;
  regs_.v           = 0;
  regs_.t           = 0;
  regs_.x           = 0;
  regs_.w           = 0;
  regs_.shift_bg_lo = 0;
  regs_.shift_bg_hi = 0;
  regs_.shift_at_lo = 0;
  regs_.shift_at_hi = 0;
  scanline_         = PRE_RENDER_SCANLINE;
  dot_              = 0;
  cycles_           = 0;
  frames_           = 0;
  ready_            = false;

  std::memset(oam_, 0, sizeof(oam_));
  std::memset(palette_, 0, sizeof(palette_));
  std::memset(vram_, 0, sizeof(vram_));
  std::memset(back_frame_.get(), 0, sizeof(256 * 240));
  std::memset(front_frame_.get(), 0, sizeof(256 * 240));
}

void Ppu::reset() {
  // N.B., nesdev.org says that internal register v is _not_ cleared on reset.
  regs_.PPUCTRL     = 0;
  regs_.PPUMASK     = 0;
  regs_.PPUDATA     = 0;
  regs_.t           = 0;
  regs_.x           = 0;
  regs_.w           = 0;
  regs_.shift_bg_lo = 0;
  regs_.shift_bg_hi = 0;
  regs_.shift_at_lo = 0;
  regs_.shift_at_hi = 0;
  scanline_         = PRE_RENDER_SCANLINE;
  dot_              = 0;
  cycles_           = 0;
  frames_           = 0;
  ready_            = false;

  std::memset(back_frame_.get(), 0, sizeof(256 * 240));
  std::memset(front_frame_.get(), 0, sizeof(256 * 240));
}

static constexpr uint16_t MMAP_ADDR_MASK    = 0x3fff;
static constexpr uint16_t PALETTE_ADDR_MASK = 0x001f;
static constexpr uint16_t PALETTE_COL_MASK  = 0x0003;
static constexpr uint16_t PALETTE_SPR_MASK  = 0xffef;

void Ppu::poke(uint16_t addr, uint8_t x) {
  addr &= MMAP_ADDR_MASK;
  if (addr < Cart::PPU_ADDR_END) {
    auto p = cart_->poke_ppu(addr, x);
    if (p.is_address()) {
      assert(p.address() < sizeof(vram_));
      vram_[p.address()] = x;
    }
  } else {
    // Map writes to sprite palette for color 0 to bg palette (color 0 is shared
    // between sprite and bg on NES).
    bool is_color_zero = !(addr & PALETTE_COL_MASK);
    if (is_color_zero) {
      addr &= PALETTE_SPR_MASK;
    }
    palette_[addr & PALETTE_ADDR_MASK] = x;
  }
}

uint8_t Ppu::peek(uint16_t addr) {
  addr &= MMAP_ADDR_MASK;
  if (addr < Cart::PPU_ADDR_END) {
    auto p = cart_->peek_ppu(addr);
    if (p.is_value()) {
      return p.value();
    } else {
      assert(p.address() < sizeof(vram_));
      return vram_[p.address()];
    }
  } else {
    // Map writes to sprite palette for color 0 to bg palette (color 0 is shared
    // between sprite and bg on NES).
    bool is_color_zero = !(addr & PALETTE_COL_MASK);
    if (is_color_zero) {
      addr &= PALETTE_SPR_MASK;
    }
    return palette_[addr & PALETTE_ADDR_MASK];
  }
}

uint8_t Ppu::read_PPUCTRL() { return read_open_bus(); }
uint8_t Ppu::read_PPUMASK() { return read_open_bus(); }
uint8_t Ppu::read_OAMADDR() { return read_open_bus(); }
uint8_t Ppu::read_PPUSCROLL() { return read_open_bus(); }
uint8_t Ppu::read_PPUADDR() { return read_open_bus(); }
uint8_t Ppu::read_OAMDMA() { return read_open_bus(); }
uint8_t Ppu::read_open_bus() { return 0; }

uint8_t Ppu::read_PPUSTATUS() {
  regs_.w   = 0;
  uint8_t r = regs_.PPUSTATUS;
  regs_.PPUSTATUS &= ~PPUSTATUS_VBLANK;
  return r;
}

uint8_t Ppu::read_OAMDATA() {
  // TODO: nesdev.org says reads during vertical or forced blanking return the
  // value from OAM at that address. What does that mean?
  return oam_[regs_.OAMADDR];
}

uint8_t Ppu::read_PPUDATA() {
  uint8_t result = regs_.PPUDATA;
  regs_.PPUDATA  = peek(regs_.v);
  if (regs_.PPUCTRL & PPUCTRL_VRAM_INC) {
    regs_.v += 32;
  } else {
    regs_.v += 1;
  }
  return result;
}

void Ppu::write_PPUCTRL(uint8_t x) {
  if (!ready_) {
    return;
  }

  bool nmi_before = regs_.PPUCTRL & PPUCTRL_NMI_ENABLE;
  bool nmi_after  = x & PPUCTRL_NMI_ENABLE;
  bool vblank     = regs_.PPUSTATUS & PPUSTATUS_VBLANK;
  if (!nmi_before && nmi_after && vblank) {
    cpu_->signal_NMI();
  }

  regs_.PPUCTRL = x;
  set_bits<V_NAME_TABLE>(regs_.t, x);

  // TODO: emulate "Bit 0 race condition" from
  // https://www.nesdev.org/wiki/PPU_registers#PPUCTRL
}

void Ppu::write_PPUMASK(uint8_t x) {
  if (!ready_) {
    return;
  }

  regs_.PPUMASK = x;

  // TODO: nesdev.org says toggling rendering takes effect approximately 3-4
  // dots after the write. This delay is required by Battletoads to avoid a
  // crash.
}

void Ppu::write_PPUSTATUS([[maybe_unused]] uint8_t x) {
  // no-op (fully accurate would be setting the "open bus" latch)
}

void Ppu::write_OAMADDR(uint8_t x) { regs_.OAMADDR = x; }

void Ppu::write_OAMDATA(uint8_t x) {
  oam_[regs_.OAMADDR] = x;
  regs_.OAMADDR++;
}

void Ppu::write_OAMDMA(uint8_t x) { regs_.OAMDMA = x; }

void Ppu::write_PPUSCROLL(uint8_t x) {
  if (!ready_) {
    return;
  }

  if (!regs_.w) {
    set_bits<V_COARSE_X>(regs_.t, x >> 3);
    regs_.x = x & 7;
    regs_.w = 1;
  } else {
    set_bits<V_COARSE_Y>(regs_.t, x >> 3);
    set_bits<V_FINE_Y>(regs_.t, x & 7);
    regs_.w = 0;
  }
}

void Ppu::write_PPUADDR(uint8_t x) {
  if (!ready_) {
    return;
  }

  if (!regs_.w) {
    set_bits<V_HI>(regs_.t, x & 0x3f);
    regs_.w = 1;
  } else {
    set_bits<V_LO>(regs_.t, x);
    regs_.v = regs_.t;
    regs_.w = 0;
  }
}

void Ppu::write_PPUDATA(uint8_t x) {
  poke(regs_.v, x);
  if (regs_.PPUCTRL & PPUCTRL_VRAM_INC) {
    regs_.v += 32;
  } else {
    regs_.v += 1;
  }
}

void Ppu::step() {
  assert(scanline_ <= PRE_RENDER_SCANLINE);
  assert(dot_ < SCANLINE_MAX_CYCLES);

  if (scanline_ < VISIBLE_FRAME_END) {
    draw_loop_step();
    bg_loop_.step();
  } else if (scanline_ == PRE_RENDER_SCANLINE) {
    if (dot_ == 1) {
      regs_.PPUSTATUS &= ~PPUSTATUS_ALL;
    }
    bg_loop_.step();
  } else if (scanline_ == 241 && dot_ == 1) {
    set_vblank();
  }

  next_dot();
  cycles_++;
}

void Ppu::next_dot() {
  // Increment dot.
  dot_++;
  if (dot_ < SCANLINE_MAX_CYCLES) {
    return;
  }

  // Dot overflow -> increment scanline.
  dot_ = 0;
  scanline_++;
  if (scanline_ < PRE_RENDER_SCANLINE) {
    return;
  } else if (scanline_ == PRE_RENDER_SCANLINE) {
    frames_++;
    back_frame_.swap(front_frame_);
    ready_ = frames_ >= 1;
    return;
  }

  // Scanline overflow -> wrap back to 0.
  scanline_      = 0;
  bool odd_frame = frames_ & 1;
  if (bg_rendering() && odd_frame) {
    dot_++;
  }
}

void Ppu::draw_loop_step() {
  if (!bg_rendering()) {
    return;
  }
  if (!(dot_ >= 2 && dot_ <= 257)) {
    return;
  }

  assert(scanline_ >= 0 && scanline_ < 240);

  // TODO: handle PPUMASK background clipping

  uint8_t bg_lo     = (regs_.shift_bg_lo >> (15 - regs_.x)) & 1;
  uint8_t bg_hi     = (regs_.shift_bg_hi >> (15 - regs_.x)) & 1;
  int     color_idx = bg_lo | (bg_hi << 1);

  uint8_t at_lo       = (regs_.shift_at_lo >> (15 - regs_.x)) & 1;
  uint8_t at_hi       = (regs_.shift_at_hi >> (15 - regs_.x)) & 1;
  int     palette_idx = at_lo | (at_hi << 1);

  uint8_t color = palette_[palette_idx * 4 + color_idx];
  back_frame_[scanline_ * 256 + dot_ - 2] = color;
}

uint8_t Ppu::fetch_nt() {
  // See https://www.nesdev.org/wiki/PPU_scrolling#Tile_and_attribute_fetching
  uint16_t addr = 0x2000 | (regs_.v & 0x0fff);
  return peek(addr);
}

uint8_t Ppu::fetch_at() {
  // See https://www.nesdev.org/wiki/PPU_scrolling#Tile_and_attribute_fetching
  uint16_t addr = 0x23c0 | (regs_.v & 0x0c00) | ((regs_.v >> 4) & 0x38) |
                  ((regs_.v >> 2) & 0x07);
  return peek(addr);
}

uint8_t Ppu::fetch_bg_lo(uint8_t nt) {
  uint16_t addr = bg_pattern_table_addr();
  addr += nt << 4;
  addr += get_bits<V_FINE_Y>(regs_.v);
  return peek(addr);
}

uint8_t Ppu::fetch_bg_hi(uint8_t nt) {
  uint16_t addr = bg_pattern_table_addr() + 8;
  addr += nt << 4;
  addr += get_bits<V_FINE_Y>(regs_.v);
  return peek(addr);
}

void Ppu::inc_v_horz() {
  bool overflow = inc_bits<V_COARSE_X, V_COARSE_X_MAX>(regs_.v);
  if (overflow) {
    regs_.v ^= V_NAME_TABLE_H;
  }
}

void Ppu::inc_v_vert() {
  bool overflow = inc_bits<V_FINE_Y, V_FINE_Y_MAX>(regs_.v);
  if (overflow) {
    overflow = inc_bits<V_COARSE_Y, V_COARSE_Y_MAX>(regs_.v);
    if (overflow) {
      regs_.v ^= V_NAME_TABLE_V;
    }
  }
}

void Ppu::set_v_horz() {
  copy_bits<V_COARSE_X | V_NAME_TABLE_H>(regs_.t, regs_.v);
}

void Ppu::set_v_vert() {
  copy_bits<V_COARSE_Y | V_FINE_Y | V_NAME_TABLE_V>(regs_.t, regs_.v);
}

void Ppu::reload_bg_regs(uint8_t at, uint8_t bg_lo, uint8_t bg_hi) {
  set_bits<0x00ff>(regs_.shift_bg_lo, bg_lo);
  set_bits<0x00ff>(regs_.shift_bg_hi, bg_hi);

  // See https://www.nesdev.org/wiki/PPU_attribute_tables
  int     y      = get_bits<V_COARSE_Y>(regs_.v) & 1;
  int     x      = get_bits<V_COARSE_X>(regs_.v) & 1;
  int     lo_idx = y * 4 + x * 2;
  int     hi_idx = lo_idx + 1;
  uint8_t lo     = (uint8_t)((at >> lo_idx) & 1);
  uint8_t hi     = (uint8_t)((at >> hi_idx) & 1);
  uint8_t lo_x8  = ~(lo - 1);
  uint8_t hi_x8  = ~(hi - 1);
  // N.B., nesdev.org says that the lo/hi bits populate one bit latches which
  // then are shifted repeatedly. For simplicity, we just make the AT shift
  // registers 16 bits (instead of 8 bits) wide and set the incoming 8 bits to
  // be repeated copies of the lo/hi bits.
  set_bits<0x00ff>(regs_.shift_at_lo, lo_x8);
  set_bits<0x00ff>(regs_.shift_at_hi, hi_x8);
}

void Ppu::shift_bg_regs() {
  regs_.shift_bg_lo <<= 1;
  regs_.shift_bg_hi <<= 1;
  regs_.shift_at_lo <<= 1;
  regs_.shift_at_hi <<= 1;
}

void Ppu::set_vblank() {
  regs_.PPUSTATUS |= PPUSTATUS_VBLANK;
  if (regs_.PPUCTRL & PPUCTRL_NMI_ENABLE) {
    cpu_->signal_NMI();
  }
}

#define SUSPEND_BG_LOOP()                                                      \
  last_dot = dot_;                                                             \
  co_await std::suspend_always();                                              \
  if (dot_ != last_dot + 1 || !bg_rendering()) {                               \
    goto bg_loop_idle;                                                         \
  }

Coroutine Ppu::bg_loop() {
  int last_dot = 0;

  while (true) {
  bg_loop_idle:
    while (dot_ != 0 || !bg_rendering()) {
      co_await std::suspend_always();
    }

    // Cycle 0 is idle.
    assert(scanline_ == 261 || scanline_ < 240);
    assert(dot_ == 0);
    bool skip_dot_0 = scanline_ == 0 && (frames_ & 1);
    if (!skip_dot_0) {
      SUSPEND_BG_LOOP();
    }

    // Cycles 1..256 fetch and load BG shift registers.
    assert(dot_ == 1);
    while (dot_ != 257) {
      uint8_t tmp = fetch_nt();
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      uint8_t nt = tmp;
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      tmp = fetch_at();
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      uint8_t at = tmp;
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      tmp = fetch_bg_lo(nt);
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      uint8_t bg_lo = tmp;
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      tmp = fetch_bg_hi(nt);
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      uint8_t bg_hi = tmp;
      inc_v_horz();
      if (dot_ == 256) {
        inc_v_vert();
      }
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      reload_bg_regs(at, bg_lo, bg_hi);
    }

    // Cycle 257 sets the vertical fields of v.
    assert(dot_ == 257);
    set_v_horz();
    SUSPEND_BG_LOOP();

    // Cycles 258..279 are idle.
    while (dot_ != 280) {
      SUSPEND_BG_LOOP();
    }

    // Cycles 280..304 set the horizontal fields of v (pre-render only).
    for (int i = 280; i <= 304; i++) {
      if (scanline_ == PRE_RENDER_SCANLINE) {
        set_v_vert();
      }
      SUSPEND_BG_LOOP();
    }

    // Cycles 305..320 are idle.
    while (dot_ != 321) {
      SUSPEND_BG_LOOP();
    }

    // Cycles 321..336 fetch and load BG shift regs (for the next scanline).
    assert(dot_ == 321);
    while (dot_ != 337) {
      uint8_t tmp = fetch_nt();
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      uint8_t nt = tmp;
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      tmp = fetch_at();
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      uint8_t at = tmp;
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      tmp = fetch_bg_lo(nt);
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      uint8_t bg_lo = tmp;
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      tmp = fetch_bg_hi(nt);
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      uint8_t bg_hi = tmp;
      inc_v_horz();
      SUSPEND_BG_LOOP();
      shift_bg_regs();
      reload_bg_regs(at, bg_lo, bg_hi);
    }
  }
}
