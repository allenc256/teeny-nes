#include <array>
#include <bit>
#include <cassert>
#include <cstring>
#include <format>

#include "src/emu/cart.h"
#include "src/emu/cpu.h"
#include "src/emu/ppu.h"

using enum Ppu::Ops;

template <uint16_t mask> constexpr uint16_t get_bits(uint16_t x) {
  constexpr int off = std::countr_zero(mask);
  return (x & mask) >> off;
}

template <uint16_t mask> constexpr void set_bits(uint16_t &bits, int x) {
  constexpr int off = std::countr_zero(mask);
  bits              = (bits & ~mask) | ((x << off) & mask);
}

template <uint16_t mask> constexpr void copy_bits(uint16_t from, uint16_t &to) {
  set_bits<mask>(to, get_bits<mask>(from));
}

template <uint16_t mask, int max> constexpr bool inc_bits(uint16_t &bits) {
  int x = get_bits<mask>(bits);
  if (x < max) {
    set_bits<mask>(bits, x + 1);
    return false;
  } else {
    set_bits<mask>(bits, 0);
    return true;
  }
}

static constexpr int SCANLINE_MAX_CYCLES = 341;

static constexpr void add_op(uint16_t &flags, Ppu::Ops op) {
  flags |= (uint16_t)(1u << op);
}

static void init_op_table_common(uint16_t *ops) {
  // Shift register ops.
  for (int i = 2; i <= 257; i++) {
    add_op(ops[i], OP_S_REG_SHIFT_BG);
    add_op(ops[i], OP_S_REG_SHIFT_SP);
  }
  for (int i = 322; i <= 337; i++) {
    add_op(ops[i], OP_S_REG_SHIFT_BG);
  }
  for (int i = 9; i <= 257; i += 8) {
    add_op(ops[i], OP_S_REG_RELOAD_BG);
  }
  add_op(ops[329], OP_S_REG_RELOAD_BG);
  add_op(ops[337], OP_S_REG_RELOAD_BG);

  // Fetch ops.
  // N.B., garbage NT fetches are NOT emulated.
  for (int i = 1; i <= 256; i += 8) {
    add_op(ops[i], OP_FETCH_NT);
    add_op(ops[i + 2], OP_FETCH_AT);
    add_op(ops[i + 4], OP_FETCH_BG_LO);
    add_op(ops[i + 6], OP_FETCH_BG_HI);
  }
  for (int i = 321; i <= 336; i += 8) {
    add_op(ops[i], OP_FETCH_NT);
    add_op(ops[i + 2], OP_FETCH_AT);
    add_op(ops[i + 4], OP_FETCH_BG_LO);
    add_op(ops[i + 6], OP_FETCH_BG_HI);
  }

  // v register ops.
  for (int i = 8; i <= 256; i += 8) {
    add_op(ops[i], OP_V_REG_INC_HORZ);
  }
  add_op(ops[256], OP_V_REG_INC_VERT);
  add_op(ops[257], OP_V_REG_SET_HORZ);
  add_op(ops[328], OP_V_REG_INC_HORZ);
  add_op(ops[336], OP_V_REG_INC_HORZ);
}

static auto init_visible_frame_op_table() {
  std::array<uint16_t, 341> ops = {0};
  init_op_table_common(ops.data());
  for (int i = 2; i <= 257; i++) {
    add_op(ops[i], OP_DRAW);
  }
  return ops;
}

static auto init_pre_render_op_table() {
  std::array<uint16_t, 341> ops = {0};
  init_op_table_common(ops.data());
  add_op(ops[1], OP_FLAG_CLEAR);
  for (int i = 280; i <= 304; i++) {
    add_op(ops[i], OP_V_REG_SET_VERT);
  }
  return ops;
}

static uint16_t init_always_enabled_op_mask() {
  uint16_t mask = 0;
  add_op(mask, OP_DRAW);
  add_op(mask, OP_FLAG_SET_VBLANK);
  add_op(mask, OP_FLAG_CLEAR);
  return mask;
}

static uint16_t init_bg_rendering_op_mask() {
  uint16_t mask = 0;
  add_op(mask, OP_S_REG_SHIFT_BG);
  add_op(mask, OP_S_REG_RELOAD_BG);
  add_op(mask, OP_FETCH_NT);
  add_op(mask, OP_FETCH_AT);
  add_op(mask, OP_FETCH_BG_LO);
  add_op(mask, OP_FETCH_BG_HI);
  add_op(mask, OP_V_REG_INC_HORZ);
  add_op(mask, OP_V_REG_INC_VERT);
  add_op(mask, OP_V_REG_SET_HORZ);
  add_op(mask, OP_V_REG_SET_VERT);
  return mask;
}

static const auto VISIBLE_FRAME_OP_TABLE = init_visible_frame_op_table();
static const auto PRE_RENDER_OP_TABLE    = init_pre_render_op_table();

static const uint16_t ALWAYS_ENABLED_OP_MASK = init_always_enabled_op_mask();
static const uint16_t BG_RENDERING_OP_MASK   = init_bg_rendering_op_mask();

Ppu::Ppu()
    : cart_(nullptr),
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
  regs_.shift_bg_hi = 0;
  regs_.shift_bg_lo = 0;
  regs_.fetch_nt    = 0;
  regs_.fetch_bg_lo = 0;
  regs_.fetch_bg_hi = 0;
  scanline_         = PRE_RENDER_SCANLINE;
  dot_              = 0;
  cycles_           = 0;
  frames_           = 0;

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
  regs_.shift_bg_hi = 0;
  regs_.shift_bg_lo = 0;
  regs_.fetch_nt    = 0;
  regs_.fetch_bg_lo = 0;
  regs_.fetch_bg_hi = 0;
  scanline_         = PRE_RENDER_SCANLINE;
  dot_              = 0;
  cycles_           = 0;
  frames_           = 0;

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
  // nesdev.org suggests ignoring writes entirely when rendering (on the
  // pre-render line and the visible lines 0–239, provided either sprite or
  // background rendering is enabled).
  if (regs_.PPUMASK & PPUMASK_RENDERING &&
      (scanline_ == PRE_RENDER_SCANLINE || scanline_ < VISIBLE_FRAME_END)) {
    return;
  }

  oam_[regs_.OAMADDR] = x;
  regs_.OAMADDR++;
}

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

void Ppu::write_OAMDMA([[maybe_unused]] uint8_t x) {
  // TODO: implement OAMDMA
}

void Ppu::step() {
  assert(scanline_ <= PRE_RENDER_SCANLINE);
  assert(dot_ < SCANLINE_MAX_CYCLES);

  if (scanline_ < VISIBLE_FRAME_END) {
    execute_ops(VISIBLE_FRAME_OP_TABLE[dot_]);
  } else if (scanline_ == PRE_RENDER_SCANLINE) {
    execute_ops(PRE_RENDER_OP_TABLE[dot_]);
  } else if (scanline_ == 241 && dot_ == 1) {
    op_flag_set_vblank(); // only possible post-render op
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

void Ppu::execute_ops(uint16_t op_mask) {
  uint16_t enabled = ALWAYS_ENABLED_OP_MASK;
  if (bg_rendering()) {
    enabled |= BG_RENDERING_OP_MASK;
  }
  op_mask &= enabled;

  while (op_mask) {
    int op = std::countr_zero(op_mask);
    switch (op) {
    case OP_DRAW: op_draw(); break;
    case OP_S_REG_SHIFT_BG: op_s_reg_shift_bg(); break;
    case OP_S_REG_SHIFT_SP: op_s_reg_shift_sp(); break;
    case OP_S_REG_RELOAD_BG: op_s_reg_reload_bg(); break;
    case OP_FETCH_NT: op_fetch_nt(); break;
    case OP_FETCH_AT: op_fetch_at(); break;
    case OP_FETCH_BG_LO: op_fetch_bg_lo(); break;
    case OP_FETCH_BG_HI: op_fetch_bg_hi(); break;
    case OP_FLAG_SET_VBLANK: op_flag_set_vblank(); break;
    case OP_FLAG_CLEAR: op_flag_clear(); break;
    case OP_V_REG_INC_HORZ: op_v_reg_inc_horz(); break;
    case OP_V_REG_INC_VERT: op_v_reg_inc_vert(); break;
    case OP_V_REG_SET_HORZ: op_v_reg_set_horz(); break;
    case OP_V_REG_SET_VERT: op_v_reg_set_vert(); break;
    default: assert(false); break;
    }
    op_mask &= op_mask - 1;
  }
}

void Ppu::op_draw() {
  if (!bg_rendering()) {
    return;
  }

  static constexpr uint8_t palette[4] = {0x0f, 0x00, 0x10, 0x20};

  uint8_t lo      = (regs_.shift_bg_lo >> (15 - regs_.x)) & 1;
  uint8_t hi      = (regs_.shift_bg_hi >> (15 - regs_.x)) & 1;
  int     pal_idx = lo | (hi << 1);
  uint8_t col     = palette[pal_idx];
  assert(dot_ >= 2 && dot_ <= 257);
  assert(scanline_ >= 0 && scanline_ < 240);
  back_frame_[scanline_ * 256 + dot_ - 2] = col;
}

void Ppu::op_s_reg_shift_bg() {
  assert(bg_rendering());
  regs_.shift_bg_lo = (uint16_t)((regs_.shift_bg_lo << 1) | 1);
  regs_.shift_bg_hi = (uint16_t)((regs_.shift_bg_hi << 1) | 1);
}

void Ppu::op_s_reg_shift_sp() {
  // not implemented
}

void Ppu::op_s_reg_reload_bg() {
  assert(bg_rendering());
  set_bits<0x00ff>(regs_.shift_bg_lo, regs_.fetch_bg_lo);
  set_bits<0x00ff>(regs_.shift_bg_hi, regs_.fetch_bg_hi);
}

void Ppu::op_fetch_nt() {
  assert(bg_rendering());
  uint16_t addr  = 0x2000 | (regs_.v & 0x0fff);
  regs_.fetch_nt = peek(addr);
}

void Ppu::op_fetch_at() {
  //
}

void Ppu::op_fetch_bg_lo() {
  assert(bg_rendering());
  uint16_t addr = bg_pattern_table_addr();
  addr += regs_.fetch_nt << 4;
  addr += get_bits<V_FINE_Y>(regs_.v);
  regs_.fetch_bg_lo = peek(addr);
}

void Ppu::op_fetch_bg_hi() {
  assert(bg_rendering());
  uint16_t addr = bg_pattern_table_addr();
  addr += regs_.fetch_nt << 4;
  addr += get_bits<V_FINE_Y>(regs_.v);
  addr += 8;
  regs_.fetch_bg_hi = peek(addr);
}

void Ppu::op_flag_set_vblank() {
  regs_.PPUSTATUS |= PPUSTATUS_VBLANK;
  // TODO: generate interrupt on VBLANK
}

void Ppu::op_flag_clear() { regs_.PPUSTATUS &= ~PPUSTATUS_ALL; }

void Ppu::op_v_reg_inc_horz() {
  assert(bg_rendering());
  bool overflow = inc_bits<V_COARSE_X, V_COARSE_X_MAX>(regs_.v);
  if (overflow) {
    regs_.v ^= V_NAME_TABLE_H;
  }
}

void Ppu::op_v_reg_inc_vert() {
  assert(bg_rendering());
  bool overflow = inc_bits<V_FINE_Y, V_FINE_Y_MAX>(regs_.v);
  if (overflow) {
    overflow = inc_bits<V_COARSE_Y, V_COARSE_Y_MAX>(regs_.v);
    if (overflow) {
      regs_.v ^= V_NAME_TABLE_V;
    }
  }
}

void Ppu::op_v_reg_set_horz() {
  assert(bg_rendering());
  copy_bits<V_COARSE_X>(regs_.t, regs_.v);
  copy_bits<V_NAME_TABLE_H>(regs_.t, regs_.v);
}

void Ppu::op_v_reg_set_vert() {
  assert(bg_rendering());
  copy_bits<V_COARSE_Y>(regs_.t, regs_.v);
  copy_bits<V_NAME_TABLE_V>(regs_.t, regs_.v);
}
