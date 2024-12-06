#include "ppu.h"

#include <cstring>

Ppu::Ppu() : cart_(nullptr), cpu_(nullptr) {}

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

  ready_ = false;
}

void Ppu::reset() {
  // N.B., nesdev.org says that internal register v is _not_ cleared on reset.
  regs_.PPUCTRL = 0;
  regs_.PPUMASK = 0;
  regs_.PPUDATA = 0;
  regs_.t       = 0;
  regs_.x       = 0;
  regs_.w       = 0;

  ready_ = false;
}

static constexpr uint16_t MMAP_ADDR_MASK    = 0x3ff;
static constexpr uint16_t PALETTE_ADDR_MASK = 0x001f;

void Ppu::poke(uint16_t addr, uint8_t x) {
  addr &= MMAP_ADDR_MASK;
  if (addr < Cartridge::PPU_ADDR_END) {
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
  if (addr < Cartridge::PPU_ADDR_END) {
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
  regs_.w = 0;
  return regs_.PPUSTATUS;
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
  if (regs_.PPUMASK & PPUMASK_RENDERING) {
    // TODO: check the scanline here!!!
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
  throw std::runtime_error("not implemented yet");
}