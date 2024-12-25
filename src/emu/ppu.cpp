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
      spr_loop_(spr_loop()),
      cart_(nullptr),
      cpu_(nullptr),
      scanline_(0),
      dot_(0),
      back_frame_(std::make_unique<uint8_t[]>(256 * 240)),
      front_frame_(std::make_unique<uint8_t[]>(256 * 240)),
      cycles_(0),
      frames_(0),
      ready_(false) {}

uint16_t Ppu::bg_pt_base_addr() const {
  return (uint16_t)((regs_.PPUCTRL & PPUCTRL_BG_ADDR) << 8);
}

bool Ppu::rendering() const { return regs_.PPUMASK & PPUMASK_RENDERING; }
bool Ppu::bg_rendering() const { return regs_.PPUMASK & PPUMASK_BG_RENDERING; }

bool Ppu::spr_rendering() const {
  return regs_.PPUMASK & PPUMASK_SPR_RENDERING;
}

bool Ppu::bg_show_left() const { return regs_.PPUMASK & PPUMASK_BG_SHOW_LEFT; }

bool Ppu::spr_show_left() const {
  return regs_.PPUMASK & PPUMASK_SPR_SHOW_LEFT;
}

int Ppu::color_emphasis() const {
  return get_bits<PPUMASK_EMPHASIS>(regs_.PPUMASK);
}

uint16_t Ppu::spr_pt_base_addr() const {
  return (uint16_t)((regs_.PPUCTRL & PPUCTRL_SPR_ADDR) << 9);
}

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
  addr_bus_         = 0;
  scanline_         = PRE_RENDER_SCANLINE;
  dot_              = 0;
  cycles_           = 0;
  frames_           = 0;
  ready_            = false;

  std::memset(oam_, 0, sizeof(oam_));
  std::memset(soam_, 0, sizeof(soam_));
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
  addr_bus_         = 0;
  scanline_         = PRE_RENDER_SCANLINE;
  dot_              = 0;
  cycles_           = 0;
  frames_           = 0;
  ready_            = false;
  bg_loop_          = bg_loop();
  spr_loop_         = spr_loop();

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
    step_visible_frame();
  } else if (scanline_ == PRE_RENDER_SCANLINE) {
    step_pre_render_scanline();
  } else {
    step_post_render_scanline();
  }

  if (!rendering() || regs_.PPUSTATUS & PPUSTATUS_VBLANK) {
    addr_bus_ = regs_.v;
  }

  if (step_cart_) {
    step_cart_ = cart_->step_ppu();
  }

  next_dot();
  cycles_++;
}

void Ppu::step_visible_frame() {
  if (dot_ >= 2 && dot_ <= 257) {
    draw_dot();
  }

  spr_loop_.step();
  bg_loop_.step();
}

void Ppu::step_pre_render_scanline() {
  if (dot_ == 1) {
    regs_.PPUSTATUS &= ~PPUSTATUS_ALL;
  }

  spr_loop_.step();
  bg_loop_.step();
}

void Ppu::step_post_render_scanline() {
  if (scanline_ == 241 && dot_ == 1) {
    regs_.PPUSTATUS |= PPUSTATUS_VBLANK;
    if (regs_.PPUCTRL & PPUCTRL_NMI_ENABLE) {
      cpu_->signal_NMI();
    }
  }
}

void Ppu::next_dot() {
  int scanline_cycles = SCANLINE_MAX_CYCLES;
  if (scanline_ == PRE_RENDER_SCANLINE && (frames_ & 1)) {
    scanline_cycles--;
  }

  // Increment dot.
  dot_++;
  if (dot_ < scanline_cycles) {
    return;
  }

  // Dot overflow -> increment scanline.
  dot_ = 0;
  scanline_++;
  if (scanline_ == VISIBLE_FRAME_END) {
    frames_++;
    back_frame_.swap(front_frame_);
    spr_buf_.clear();
    // TODO: this might be too early for marking the PPU ready
    ready_ = true;
  }
  if (scanline_ <= PRE_RENDER_SCANLINE) {
    return;
  }

  // Scanline overflow -> wrap back to 0.
  scanline_ = 0;
}

void Ppu::draw_dot() {
  assert(scanline_ >= 0 && scanline_ < 240);
  assert(dot_ >= 2 && dot_ <= 257);

  int x            = dot_ - 2;
  int frame_offset = scanline_ * 256 + x;

  if (!rendering()) {
    back_frame_[frame_offset] = palette_[0];
    return;
  }

  int  pat           = 0;
  int  pal           = 0;
  bool spr_behind    = false;
  bool spr0_rendered = false;

  if (spr_rendering() && (spr_show_left() || x >= 8)) {
    spr_buf_.get(x, pat, pal, spr_behind, spr0_rendered);
    pal += 4;
  }

  if (bg_rendering() && (bg_show_left() || x >= 8)) {
    uint8_t bg_lo  = (regs_.shift_bg_lo >> (15 - regs_.x)) & 1;
    uint8_t bg_hi  = (regs_.shift_bg_hi >> (15 - regs_.x)) & 1;
    int     bg_pat = bg_lo | (bg_hi << 1);
    if (bg_pat) {
      // Sprite 0 hit cannot happen on x = 255 for obscure reasons.
      // See https://www.nesdev.org/wiki/PPU_OAM#Sprite_0_hits
      if (spr0_rendered && x != 255) {
        // TODO: nesdev says sprite 0 hit cannot happen on x=0..7 if clipping
        regs_.PPUSTATUS |= PPUSTATUS_SPR0_HIT;
      }
      if (spr_behind || pat == 0) {
        uint8_t at_lo = (regs_.shift_at_lo >> (15 - regs_.x)) & 1;
        uint8_t at_hi = (regs_.shift_at_hi >> (15 - regs_.x)) & 1;
        pat           = bg_pat;
        pal           = at_lo | (at_hi << 1);
      }
    }
  }

  uint8_t color;
  if (pat) {
    color = palette_[pal * 4 + pat];
  } else {
    color = palette_[0];
  }
  back_frame_[frame_offset] = color;
}

uint8_t Ppu::bg_loop_fetch_nt() {
  // See https://www.nesdev.org/wiki/PPU_scrolling#Tile_and_attribute_fetching
  addr_bus_ = 0x2000 | (regs_.v & 0x0fff);
  return peek(addr_bus_);
}

uint8_t Ppu::bg_loop_fetch_at() {
  // See https://www.nesdev.org/wiki/PPU_scrolling#Tile_and_attribute_fetching
  addr_bus_ = 0x23c0 | (regs_.v & 0x0c00) | ((regs_.v >> 4) & 0x38) |
              ((regs_.v >> 2) & 0x07);
  uint8_t at = peek(addr_bus_);

  // See https://www.nesdev.org/wiki/PPU_attribute_tables
  int y      = (get_bits<V_COARSE_Y>(regs_.v) >> 1) & 1;
  int x      = (get_bits<V_COARSE_X>(regs_.v) >> 1) & 1;
  int lo_idx = y * 4 + x * 2;
  int hi_idx = lo_idx + 1;
  int lo     = (at >> lo_idx) & 1;
  int hi     = (at >> hi_idx) & 1;
  return (uint8_t)(lo | (hi << 1));
}

uint8_t Ppu::bg_loop_fetch_pt_lo(uint8_t nt) {
  uint16_t addr = bg_pt_base_addr();
  addr += nt << 4;
  addr += get_bits<V_FINE_Y>(regs_.v);
  addr_bus_ = addr;
  return peek(addr);
}

uint8_t Ppu::bg_loop_fetch_pt_hi(uint8_t nt) {
  uint16_t addr = bg_pt_base_addr() + 8;
  addr += nt << 4;
  addr += get_bits<V_FINE_Y>(regs_.v);
  addr_bus_ = addr;
  return peek(addr);
}

void Ppu::bg_loop_inc_v_horz() {
  bool overflow = inc_bits<V_COARSE_X, V_COARSE_X_MAX>(regs_.v);
  if (overflow) {
    regs_.v ^= V_NAME_TABLE_H;
  }
}

void Ppu::bg_loop_inc_v_vert() {
  bool overflow = inc_bits<V_FINE_Y, V_FINE_Y_MAX>(regs_.v);
  if (overflow) {
    overflow = inc_bits<V_COARSE_Y, V_COARSE_Y_MAX>(regs_.v);
    if (overflow) {
      regs_.v ^= V_NAME_TABLE_V;
    }
  }
}

void Ppu::bg_loop_set_v_horz() {
  copy_bits<V_COARSE_X | V_NAME_TABLE_H>(regs_.t, regs_.v);
}

void Ppu::bg_loop_set_v_vert() {
  copy_bits<V_COARSE_Y | V_FINE_Y | V_NAME_TABLE_V>(regs_.t, regs_.v);
}

void Ppu::bg_loop_reload_regs(uint8_t at, uint8_t pt_lo, uint8_t pt_hi) {
  set_bits<0x00ff>(regs_.shift_bg_lo, pt_lo);
  set_bits<0x00ff>(regs_.shift_bg_hi, pt_hi);

  assert(at >= 0 && at < 4);
  uint8_t lo    = at & 1;
  uint8_t hi    = at >> 1;
  uint8_t lo_x8 = ~(lo - 1);
  uint8_t hi_x8 = ~(hi - 1);
  // N.B., nesdev.org says that the lo/hi bits populate one bit latches which
  // then are shifted repeatedly. For simplicity, we just make the AT shift
  // registers 16 bits (instead of 8 bits) wide and set the incoming 8 bits
  // be repeated copies of the lo/hi bits.
  set_bits<0x00ff>(regs_.shift_at_lo, lo_x8);
  set_bits<0x00ff>(regs_.shift_at_hi, hi_x8);
}

void Ppu::bg_loop_shift_regs() {
  regs_.shift_bg_lo <<= 1;
  regs_.shift_bg_hi <<= 1;
  regs_.shift_at_lo <<= 1;
  regs_.shift_at_hi <<= 1;
}

static constexpr std::suspend_always SUSPEND_ALWAYS;

Coroutine Ppu::bg_loop() {
  uint8_t nt = 0, at = 0, pt_lo = 0, pt_hi = 0;

  while (true) {
    // Cycle 0 is idle.
    assert(scanline_ == 261 || scanline_ < 240);
    assert(dot_ == 0);
    if (rendering()) {
      // Nesdev says the value of the address bus should be the same as the PT
      // fetch that happens later on dot 5. For simplicity, we just set it to
      // the base PT address. Emulating this behavior seems to be necessary for
      // the MMC3 A12/IRQ scanline counter to operate properly on Mega Man 3
      // (specifically the status bar on Gemini Man's stage).
      addr_bus_ = bg_pt_base_addr();
    }
    co_await SUSPEND_ALWAYS;

    // Cycles 1..256 fetch and load BG shift registers.
    assert(dot_ == 1);
    while (dot_ != 257) {
      if (rendering()) {
        nt = bg_loop_fetch_nt();
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
        at = bg_loop_fetch_at();
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
        pt_lo = bg_loop_fetch_pt_lo(nt);
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
        pt_hi = bg_loop_fetch_pt_hi(nt);
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
        bg_loop_inc_v_horz();
        if (dot_ == 256) {
          bg_loop_inc_v_vert();
        }
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
        bg_loop_reload_regs(at, pt_lo, pt_hi);
      }
    }

    // Cycles 257..320 contain garbage NT fetches.
    // Emulating the garbage NT seems to be necessary for the MMC3 A12/IRQ
    // scanline counter to operate properly on Mega Man 3 (specifically the
    // status bar on Gemini Man's stage).
    assert(dot_ == 257);
    while (dot_ != 321) {
      if (rendering()) {
        int rel_dot = (dot_ - 257) & 0x7;
        if (rel_dot == 0 || rel_dot == 1) {
          bg_loop_fetch_nt();
        }
        if (dot_ == 257) {
          bg_loop_set_v_horz();
        }
        if (dot_ >= 280 && dot_ <= 304 && scanline_ == PRE_RENDER_SCANLINE) {
          bg_loop_set_v_vert();
        }
      }
      co_await SUSPEND_ALWAYS;
    }

    // Cycles 321..336 fetch and load BG shift regs (for the next scanline).
    assert(dot_ == 321);
    while (dot_ != 337) {
      if (rendering()) {
        nt = bg_loop_fetch_nt();
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
        at = bg_loop_fetch_at();
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
        pt_lo = bg_loop_fetch_pt_lo(nt);
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
        pt_hi = bg_loop_fetch_pt_hi(nt);
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
        bg_loop_inc_v_horz();
      }
      co_await SUSPEND_ALWAYS;
      if (rendering()) {
        bg_loop_shift_regs();
        bg_loop_reload_regs(at, pt_lo, pt_hi);
      }
    }

    // Cycles 337..340 are garbage NT fetches.
    // nesdev says these are used by MMC5 to clock a counter.
    assert(dot_ == 337);
    if (rendering()) {
      bg_loop_fetch_nt();
    }
    co_await SUSPEND_ALWAYS;
    co_await SUSPEND_ALWAYS;
    if (rendering()) {
      bg_loop_fetch_nt();
    }
    while (dot_ != 0) {
      co_await SUSPEND_ALWAYS;
    }
  }
}

static bool spr_y_in_range(int spr_y, int scanline, int spr_height) {
  return scanline >= spr_y && scanline < spr_y + spr_height;
}

static uint16_t spr_calc_pt_addr(
    int      rel_y,
    uint8_t  tile_index,
    bool     size_8x16,
    bool     flip_vert,
    uint16_t base_pt_addr
) {
  if (size_8x16) {
    assert(rel_y >= 0 && rel_y < 16);
    if (flip_vert) {
      rel_y = 15 - rel_y;
    }
    base_pt_addr = (uint16_t)((tile_index & 1) << 12);
    tile_index   = (uint8_t)((tile_index & 0xfe) | (rel_y >> 3));
    rel_y        = rel_y & 7;
  } else {
    assert(rel_y >= 0 && rel_y < 8);
    if (flip_vert) {
      rel_y = 7 - rel_y;
    }
  }
  return (uint16_t)(base_pt_addr + (tile_index << 4) + rel_y);
}

// Reference: https://forums.nesdev.org/viewtopic.php?t=15870
Coroutine Ppu::spr_loop() {
  while (true) {
    // Cycle 0 is idle.
    assert(
        scanline_ >= 0 && scanline_ < 240 || scanline_ == PRE_RENDER_SCANLINE
    );
    assert(dot_ == 0);
    co_await SUSPEND_ALWAYS;

    // Cycles 1..64 clear the secondary OAM.
    assert(dot_ == 1);
    if (scanline_ != PRE_RENDER_SCANLINE) {
      for (int i = 0; i < 32; i++) {
        co_await SUSPEND_ALWAYS;
        if (rendering()) {
          soam_[i] = 0xff;
        }
        co_await SUSPEND_ALWAYS;
      }
    } else {
      for (int i = 0; i < 64; i++) {
        co_await SUSPEND_ALWAYS;
      }
    }

    // Cycles 65..256 are sprite evaluation.
    assert(dot_ == 65);
    bool spr_size_8x16 = regs_.PPUCTRL & PPUCTRL_SPR_SIZE;
    int  spr_height    = spr_size_8x16 ? 16 : 8;
    bool spr0_enabled  = false;
    if (scanline_ != PRE_RENDER_SCANLINE) {
      for (int i = 0, soam_index = 0; i < 256; i += 4) {
        uint8_t y = oam_[i];
        co_await SUSPEND_ALWAYS;
        if (rendering()) {
          soam_[soam_index] = y;
        }
        co_await SUSPEND_ALWAYS;
        if (rendering() && spr_y_in_range(y, scanline_, spr_height)) {
          if (i == 0) {
            spr0_enabled = true;
          }
          co_await SUSPEND_ALWAYS;
          soam_[++soam_index] = oam_[i + 1];
          co_await SUSPEND_ALWAYS;
          co_await SUSPEND_ALWAYS;
          soam_[++soam_index] = oam_[i + 2];
          co_await SUSPEND_ALWAYS;
          co_await SUSPEND_ALWAYS;
          soam_[++soam_index] = oam_[i + 3];
          co_await SUSPEND_ALWAYS;
          ++soam_index;

          // TODO: implement "correct" buggy sprite overflow.
          if (soam_index >= 32) {
            regs_.PPUSTATUS |= PPUSTATUS_SPR_OVF;
            break;
          }
        }
      }
    }

    // Leftover cycles from sprite evaluation are idle.
    while (dot_ != 257) {
      co_await SUSPEND_ALWAYS;
    }

    // Cycles 257..320 are sprite tile fetches and render.
    assert(dot_ == 257);
    spr_buf_.clear();
    for (int soam_index = 0; soam_index < 32; soam_index += 4) {
      for (int i = 0; i < 4; i++) {
        co_await SUSPEND_ALWAYS;
      }
      uint8_t y = soam_[soam_index];
      if (rendering() && spr_y_in_range(y, scanline_, spr_height)) {
        uint8_t tile_idx = soam_[soam_index + 1];
        uint8_t attr     = soam_[soam_index + 2];
        uint8_t x        = soam_[soam_index + 3];
        addr_bus_        = spr_calc_pt_addr(
            scanline_ - y,
            tile_idx,
            spr_size_8x16,
            attr & SPR_ATTR_FLIP_VERT,
            spr_pt_base_addr()
        );
        uint8_t pt_lo = peek(addr_bus_);
        co_await SUSPEND_ALWAYS;
        co_await SUSPEND_ALWAYS;
        addr_bus_ += 8;
        uint8_t pt_hi = peek(addr_bus_);
        co_await SUSPEND_ALWAYS;
        co_await SUSPEND_ALWAYS;
        bool spr0 = spr0_enabled && soam_index == 0;
        spr_loop_render(x, attr, pt_lo, pt_hi, spr0);
      } else {
        // Need to ensure the address bus changes here for MMC3 A12/IRQ counter
        // compatibility.
        addr_bus_ = spr_pt_base_addr();
        for (int i = 0; i < 4; i++) {
          co_await SUSPEND_ALWAYS;
        }
      }
    }

    // Remaining cycles are idle.
    while (dot_ != 0) {
      co_await SUSPEND_ALWAYS;
    }
  }
}

void Ppu::spr_loop_render(
    int x, uint8_t attr, uint8_t pt_lo, uint8_t pt_hi, bool spr0
) {
  assert(x >= 0 && x < 256);
  int  end       = std::min(x + 7, 255);
  int  pal       = attr & SPR_ATTR_PALETTE;
  bool prio      = attr & SPR_ATTR_PRIO;
  bool flip_horz = attr & SPR_ATTR_FLIP_HORZ;
  for (int i = 0; x + i <= end; i++) {
    bool lo, hi;
    if (flip_horz) {
      lo = pt_lo & (1u << i);
      hi = pt_hi & (1u << i);
    } else {
      lo = pt_lo & (1u << (7 - i));
      hi = pt_hi & (1u << (7 - i));
    }
    int pat = lo + (hi << 1);
    spr_buf_.render(x + i, pat, pal, prio, spr0);
  }
}

SpriteBuf::SpriteBuf() { clear(); }

void SpriteBuf::clear() { std::memset(bytes_, 0, sizeof(bytes_)); }

void SpriteBuf::render(
    int x, int pattern, int palette, bool &behind, bool &spr0
) {
  assert(x >= 0 && x < 256);
  assert(pattern >= 0 && pattern < 4);
  assert(palette >= 0 && palette < 4);
  if (bytes_[x] || !pattern) {
    return;
  }
  bytes_[x] = (uint8_t)(pattern | (palette << 2) | (behind << 4) | (spr0 << 5));
}

void SpriteBuf::get(int x, int &pattern, int &palette, bool &behind, bool &spr0)
    const {
  assert(x >= 0 && x < 256);
  pattern = bytes_[x] & 3;
  palette = (bytes_[x] >> 2) & 3;
  behind  = (bytes_[x] >> 4) & 1;
  spr0    = (bytes_[x] >> 5) & 1;
}
