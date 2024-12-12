#include <array>
#include <cassert>
#include <cstring>
#include <format>

#include "src/emu/cart.h"
#include "src/emu/cpu.h"
#include "src/emu/ppu.h"

using enum Ppu::Ops;

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
  add_op(ops[257], OP_V_REG_SET_VERT);
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
      cycles_(0),
      frames_(0),
      ready_(false) {}

uint16_t Ppu::bg_pattern_table_addr() const {
  return (uint16_t)((regs_.PPUCTRL & PPUCTRL_BG_ADDR) << 8);
}

bool Ppu::bg_rendering() const { return regs_.PPUMASK & PPUMASK_BG_RENDERING; }

int  Ppu::fine_x_scroll() const { return regs_.x; }
int  Ppu::fine_y_scroll() const { return regs_.v >> 12; }
int  Ppu::coarse_x_scroll() const { return regs_.v & 0x001f; }
int  Ppu::coarse_y_scroll() const { return (regs_.v >> 5) & 0x001f; }
void Ppu::set_fine_x_scroll(int x) { regs_.x = x & 7; }
void Ppu::next_horz_name_table() { regs_.v ^= 0x0400; }
void Ppu::next_vert_name_table() { regs_.v ^= 0x0800; }

void Ppu::set_fine_y_scroll(int y) {
  regs_.v = (uint16_t)((regs_.v & 0x0fff) | ((y & 7) << 12));
}

void Ppu::set_coarse_x_scroll(int x) {
  regs_.v = (uint16_t)((regs_.v & 0x7fe0) | (x & 31));
}

void Ppu::set_coarse_y_scroll(int y) {
  regs_.v = (uint16_t)((regs_.v & 0x7c1f) | ((y & 31) << 5));
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
  std::memset(frame_bufs_, 0, sizeof(frame_bufs_));
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

  std::memset(frame_bufs_, 0, sizeof(frame_bufs_));
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
  throw std::runtime_error("not implemented yet");
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

  uint16_t t = regs_.t;
  t &= 0x73ff;
  t |= (x & 3) << 10;
  regs_.t = t;

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
    uint16_t t = regs_.t;
    t &= 0x7fe0;
    t |= x >> 3;
    regs_.t = t;
    regs_.x = x & 7;
    regs_.w = 1;
  } else {
    uint16_t t = regs_.t;
    t &= 0x0c1f;
    t |= (x & 0b11000000) << 2;
    t |= (x & 0b00111000) << 2;
    t |= (x & 0b00000111) << 12;
    regs_.t = t;
    regs_.w = 0;
  }
}

void Ppu::write_PPUADDR(uint8_t x) {
  if (!ready_) {
    return;
  }

  if (!regs_.w) {
    regs_.t = (uint16_t)((regs_.t & 0x00ff) | ((x & 0x3f) << 8));
    regs_.w = 1;
  } else {
    regs_.t = (regs_.t & 0xff00) | x;
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

  // Overflow -> increment scanline.
  dot_ = 0;
  scanline_++;
  if (scanline_ < PRE_RENDER_SCANLINE) {
    return;
  } else if (scanline_ == PRE_RENDER_SCANLINE) {
    frames_++;
    ready_ = frames_ >= 1;
    return;
  }

  // Overflow -> wrap back to 0.
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
  // not implemented
}

void Ppu::op_s_reg_shift_bg() {
  assert(bg_rendering());
  regs_.shift_bg_lo = (regs_.shift_bg_lo >> 1) | 0x8000;
  regs_.shift_bg_hi = (regs_.shift_bg_hi >> 1) | 0x8000;
}

void Ppu::op_s_reg_shift_sp() {
  // not implemented
}

void Ppu::op_s_reg_reload_bg() {
  assert(bg_rendering());
  regs_.shift_bg_lo =
      (uint16_t)((regs_.shift_bg_lo & 0xff) | (regs_.fetch_bg_lo << 8));
  regs_.shift_bg_hi =
      (uint16_t)((regs_.shift_bg_hi & 0xff) | (regs_.fetch_bg_hi << 8));
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
  addr += fine_y_scroll();
  regs_.fetch_bg_lo = peek(addr);
}

void Ppu::op_fetch_bg_hi() {
  assert(bg_rendering());
  uint16_t addr = bg_pattern_table_addr();
  addr += regs_.fetch_nt << 4;
  addr += fine_y_scroll();
  addr += 8;
  regs_.fetch_bg_lo = peek(addr);
}

void Ppu::op_flag_set_vblank() {
  regs_.PPUSTATUS |= PPUSTATUS_VBLANK;
  // TODO: generate interrupt on VBLANK
}

void Ppu::op_flag_clear() { regs_.PPUSTATUS &= ~PPUSTATUS_ALL; }

void Ppu::op_v_reg_inc_horz() {
  assert(bg_rendering());

  // Increment coarse x.
  int x = coarse_x_scroll();
  if (x < 31) {
    regs_.v++;
    return;
  }

  // Coarse x overflow.
  set_coarse_x_scroll(0);
  next_horz_name_table();
}

void Ppu::op_v_reg_inc_vert() {
  assert(bg_rendering());

  // Increment fine y.
  int y0 = fine_y_scroll();
  if (y0 < 7) {
    set_fine_y_scroll(y0 + 1);
    return;
  }

  // Fine y overflow -> increment coarse y.
  set_fine_y_scroll(0);
  int y1 = coarse_y_scroll();
  if (y1 < 29) {
    set_coarse_y_scroll(y1 + 1);
    return;
  }

  // Coarse y overflow.
  set_coarse_y_scroll(0);
  next_vert_name_table();
}

void Ppu::op_v_reg_set_horz() {}
void Ppu::op_v_reg_set_vert() {}
