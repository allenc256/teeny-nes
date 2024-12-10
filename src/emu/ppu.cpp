#include "emu/ppu.h"
#include "emu/cart.h"
#include "emu/cpu.h"

#include <array>
#include <cassert>
#include <cstring>
#include <format>

static constexpr int SCANLINE_MAX_CYCLES = 341;

// Reference: https://www.nesdev.org/wiki/PPU_rendering#Frame_timing_diagram
static constexpr auto init_visible_frame_cycles() {
  std::array<Ppu::CycleOps, SCANLINE_MAX_CYCLES> cycles;

  for (int i = 8; i < 256; i += 8) {
    cycles[i].v_op = Ppu::V_OP_INC_HORZ;
  }
  cycles[256].v_op = Ppu::V_OP_INC_HORZ;
  cycles[257].v_op = Ppu::V_OP_SET_HORZ;
  cycles[328].v_op = Ppu::V_OP_INC_HORZ;
  cycles[336].v_op = Ppu::V_OP_INC_HORZ;

  for (int i = 0; i < 256; i += 8) {
    for (int j = 1; j <= 8; j++) {
      cycles[i + j].fetch_op = (Ppu::FetchOp)j;
    }
  }
  for (int i = 320; i < 336; i += 8) {
    for (int j = 1; j <= 8; j++) {
      cycles[i + j].fetch_op = (Ppu::FetchOp)j;
    }
  }
  cycles[337].fetch_op = Ppu::FETCH_OP_NT_BYTE_CYC1;
  cycles[338].fetch_op = Ppu::FETCH_OP_NT_BYTE_CYC2;
  cycles[339].fetch_op = Ppu::FETCH_OP_NT_BYTE_CYC1;
  cycles[340].fetch_op = Ppu::FETCH_OP_NT_BYTE_CYC2;

  return cycles;
}

// Reference: https://www.nesdev.org/wiki/PPU_rendering#Frame_timing_diagram
static constexpr auto init_pre_render_cycles() {
  auto cycles = init_visible_frame_cycles();
  for (int i = 280; i < 305; i++) {
    cycles[i].v_op = Ppu::V_OP_SET_VERT;
  }
  return cycles;
}

static constexpr auto VISIBLE_FRAME_CYCLES    = init_visible_frame_cycles();
static constexpr auto PRE_RENDER_FRAME_CYCLES = init_pre_render_cycles();

Ppu::Ppu()
    : cart_(nullptr),
      cpu_(nullptr),
      scanline_(0),
      scanline_cycle_(0),
      cycles_(0),
      frames_(0),
      ready_(false) {}

void Ppu::power_up() {
  regs_.PPUCTRL   = 0;
  regs_.PPUMASK   = 0;
  regs_.PPUSTATUS = 0b10100000;
  regs_.OAMADDR   = 0;
  regs_.PPUDATA   = 0;
  regs_.v         = 0;
  regs_.t         = 0;
  regs_.x         = 0;
  regs_.w         = 0;

  std::memset(oam_, 0, sizeof(oam_));
  std::memset(palette_, 0, sizeof(palette_));
  std::memset(vram_, 0, sizeof(vram_));

  scanline_       = PRE_RENDER_SCANLINE;
  scanline_cycle_ = 0;
  cycles_         = 0;
  frames_         = 0;
}

void Ppu::reset() {
  // N.B., nesdev.org says that internal register v is _not_ cleared on reset.
  regs_.PPUCTRL = 0;
  regs_.PPUMASK = 0;
  regs_.PPUDATA = 0;
  regs_.t       = 0;
  regs_.x       = 0;
  regs_.w       = 0;

  scanline_       = PRE_RENDER_SCANLINE;
  scanline_cycle_ = 0;
  cycles_         = 0;
  frames_         = 0;
}

static constexpr uint16_t MMAP_ADDR_MASK    = 0x3fff;
static constexpr uint16_t PALETTE_ADDR_MASK = 0x001f;

void Ppu::poke(uint16_t addr, uint8_t x) {
  addr &= MMAP_ADDR_MASK;
  if (addr < Cart::PPU_ADDR_END) {
    auto p = cart_->poke_ppu(addr, x);
    if (p.is_address()) {
      assert(p.address() < sizeof(vram_));
      vram_[p.address()] = x;
    }
  } else {
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
  t &= 0b0111001111111111;
  t |= (x & 0b00000011) << 10;
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
    t &= 0b111111111100000;
    t |= x >> 3;
    regs_.t = t;
    regs_.x = x & 0b111;
    regs_.w = 1;
  } else {
    uint16_t t = regs_.t;
    t &= 0b000110000011111;
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
    regs_.t = (uint16_t)((regs_.t & 0x00ff) | ((x & 0b00111111) << 8));
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
  assert(scanline_cycle_ < SCANLINE_MAX_CYCLES);

  if (scanline_ < VISIBLE_FRAME_END) {
    step_visible_frame();
  } else if (scanline_ < PRE_RENDER_SCANLINE) {
    step_post_render();
  } else {
    step_pre_render();
  }

  cycles_++;
}

void Ppu::step_pre_render() {
  step_internal(PRE_RENDER_FRAME_CYCLES[scanline_cycle_]);

  if (scanline_cycle_ == 1) {
    clear_flags();
  }

  scanline_cycle_++;
  if (scanline_cycle_ == SCANLINE_MAX_CYCLES - (int)(frames_ & 1)) {
    scanline_       = 0;
    scanline_cycle_ = 0;
  }
}

void Ppu::step_post_render() {
  if (scanline_ == 241 && scanline_cycle_ == 1) {
    set_vblank();
  }

  scanline_cycle_++;
  if (scanline_cycle_ == SCANLINE_MAX_CYCLES) {
    scanline_++;
    scanline_cycle_ = 0;
    if (scanline_ == PRE_RENDER_SCANLINE) {
      frames_++;
      ready_ = frames_ >= 1;
    }
  }
}

void Ppu::step_visible_frame() {
  step_internal(VISIBLE_FRAME_CYCLES[scanline_cycle_]);

  if (scanline_cycle_ >= 16 && scanline_cycle_ < 16 + 256) {
    draw_pixel();
  }

  scanline_cycle_++;
  if (scanline_cycle_ == SCANLINE_MAX_CYCLES) {
    scanline_++;
    scanline_cycle_ = 0;
  }
}

void Ppu::step_internal(CycleOps ops) {
  switch (ops.v_op) {
  case V_OP_INC_HORZ: inc_v_horizontal(); break;
  case V_OP_INC_VERT: inc_v_vertical(); break;
  case V_OP_SET_HORZ: set_v_horizontal(); break;
  case V_OP_SET_VERT: set_v_vertical(); break;
  default: assert(ops.v_op == V_OP_NONE); // no-op
  }

  switch (ops.fetch_op) {
  case FETCH_OP_NT_BYTE_CYC1: fetch_NT_byte_cyc1(); break;
  case FETCH_OP_NT_BYTE_CYC2: fetch_NT_byte_cyc2(); break;
  case FETCH_OP_AT_BYTE_CYC1: fetch_AT_byte_cyc1(); break;
  case FETCH_OP_AT_BYTE_CYC2: fetch_AT_byte_cyc2(); break;
  case FETCH_OP_PT_BYTE_LO_CYC1: fetch_PT_byte_lo_cyc1(); break;
  case FETCH_OP_PT_BYTE_LO_CYC2: fetch_PT_byte_lo_cyc2(); break;
  case FETCH_OP_PT_BYTE_HI_CYC1: fetch_PT_byte_hi_cyc1(); break;
  case FETCH_OP_PT_BYTE_HI_CYC2: fetch_PT_byte_hi_cyc2(); break;
  default: assert(ops.fetch_op == FETCH_OP_NONE); // no-op
  }
}

void Ppu::set_vblank() { regs_.PPUSTATUS |= PPUSTATUS_VBLANK; }
void Ppu::clear_flags() { regs_.PPUSTATUS &= ~PPUSTATUS_ALL; }

void Ppu::fetch_NT_byte_cyc1() {}
void Ppu::fetch_NT_byte_cyc2() {}
void Ppu::fetch_AT_byte_cyc1() {}
void Ppu::fetch_AT_byte_cyc2() {}
void Ppu::fetch_PT_byte_lo_cyc1() {}
void Ppu::fetch_PT_byte_lo_cyc2() {}
void Ppu::fetch_PT_byte_hi_cyc1() {}
void Ppu::fetch_PT_byte_hi_cyc2() {}
void Ppu::inc_v_horizontal() {}
void Ppu::inc_v_vertical() {}
void Ppu::set_v_horizontal() {}
void Ppu::set_v_vertical() {}
void Ppu::draw_pixel() {}