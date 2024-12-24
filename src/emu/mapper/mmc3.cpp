#include <cstring>

#include "src/emu/cpu.h"
#include "src/emu/cycles.h"
#include "src/emu/mapper/mmc3.h"
#include "src/emu/ppu.h"

Mmc3::Mmc3(const Header &header, Memory &&mem)
    : mem_(std::move(mem)),
      cpu_(nullptr) {
  if (header.mirroring_specified()) {
    orig_mirroring_ = header.mirroring();
  } else {
    orig_mirroring_ = MIRROR_HORZ;
  }
}

void Mmc3::set_cpu(Cpu *cpu) { cpu_ = cpu; }
void Mmc3::set_ppu(Ppu *ppu) { ppu_ = ppu; }
void Mmc3::power_up() { reset(); }

void Mmc3::reset() {
  std::memset(&regs_, 0, sizeof(regs_));
  std::memset(&irq_, 0, sizeof(irq_));
  mirroring_ = orig_mirroring_;
}

int Mmc3::prg_rom_banks() const { return mem_.prg_rom_size >> 13; }
int Mmc3::chr_rom_banks() const { return mem_.chr_rom_size >> 10; }

uint8_t Mmc3::peek_cpu(uint16_t addr) {
  if (addr >= 0x8000) {
    return mem_.prg_rom[map_prg_rom_addr(addr)];
  } else if (addr >= 0x6000) {
    return mem_.prg_ram[addr - 0x6000];
  } else {
    return 0;
  }
}

void Mmc3::poke_cpu(uint16_t addr, uint8_t x) {
  if (addr < 0x6000) {
    return;
  } else if (addr < 0x8000) {
    mem_.prg_ram[addr - 0x6000] = x;
    return;
  }

  int  region = (addr - 0x8000) >> 13;
  bool even   = !(addr & 1);
  assert(region >= 0 && region < 4);
  switch (region) {
  case 0: // 0x8000..0x9fff
    if (even) {
      regs_.bank_select = x;
    } else {
      write_bank_data(x);
    }
    break;
  case 1: // 0xa000..0xbfff
    if (even) {
      write_mirroring(x);
    } else {
      // TODO: PRG RAM protect
    }
    break;
  case 2: // 0xc000..0xdfff
    if (even) {
      irq_.latch = x;
    } else {
      irq_.reload = true;
    }
    break;
  default: // 0xe000..0xffff
    if (even) {
      irq_.enabled = false;
      cpu_->clear_IRQ();
    } else {
      irq_.enabled = true;
    }
    break;
  }
}

void Mmc3::write_mirroring(uint8_t x) {
  if (x & 1) {
    mirroring_ = MIRROR_HORZ;
  } else {
    mirroring_ = MIRROR_VERT;
  }
}

void Mmc3::write_bank_data(uint8_t x) {
  int index = regs_.bank_select & 7;
  switch (index) {
  case 0:
  case 1: x &= 0b11111110; // falls-through
  case 2:
  case 3:
  case 4:
  case 5: x %= chr_rom_banks(); break;
  case 6:
  default:
    x &= 0b00111111;
    x %= prg_rom_banks();
    break;
  }
  regs_.R[index] = x;
}

Mmc3::PeekPpu Mmc3::peek_ppu(uint16_t addr) {
  if (addr < 0x2000) {
    return PeekPpu::make_value(mem_.chr_rom[map_chr_rom_addr(addr)]);
  } else if (addr < 0x3000) {
    return PeekPpu::make_address(mirrored_nt_addr(mirroring_, addr));
  } else {
    return PeekPpu::make_value(0);
  }
}

Mmc3::PokePpu Mmc3::poke_ppu(uint16_t addr, uint8_t x) {
  if (addr < 0x2000) {
    if (!mem_.chr_rom_readonly) {
      mem_.chr_rom[map_chr_rom_addr(addr)] = x;
    }
    return PokePpu::make_success();
  } else if (addr < 0x3000) {
    return PokePpu::make_address(mirrored_nt_addr(mirroring_, addr));
  } else {
    return PokePpu::make_success();
  }
}

int Mmc3::map_prg_rom_addr(uint16_t cpu_addr) {
  int bank;
  int region = (cpu_addr - 0x8000) >> 13;
  assert(region >= 0 && region < 4);
  if (!(regs_.bank_select & 0b01000000)) {
    switch (region) {
    case 0: bank = regs_.R[6]; break;
    case 1: bank = regs_.R[7]; break;
    case 2: bank = prg_rom_banks() - 2; break;
    default: bank = prg_rom_banks() - 1; break;
    }
  } else {
    switch (region) {
    case 0: bank = prg_rom_banks() - 2; break;
    case 1: bank = regs_.R[7]; break;
    case 2: bank = regs_.R[6]; break;
    default: bank = prg_rom_banks() - 1; break;
    }
  }
  int offset   = cpu_addr & 0x1fff;
  int prg_addr = (bank << 13) + offset;
  assert(prg_addr < mem_.prg_rom_size);
  return prg_addr;
}

int Mmc3::map_chr_rom_addr(uint16_t ppu_addr) {
  int region = ppu_addr >> 10;
  int offset = ppu_addr & 1023;
  int bank;
  assert(region >= 0 && region < 8);
  if (!(regs_.bank_select & 0b10000000)) {
    switch (region) {
    case 0: bank = regs_.R[0]; break;
    case 1: bank = regs_.R[0] + 1; break;
    case 2: bank = regs_.R[1]; break;
    case 3: bank = regs_.R[1] + 1; break;
    case 4: bank = regs_.R[2]; break;
    case 5: bank = regs_.R[3]; break;
    case 6: bank = regs_.R[4]; break;
    default: bank = regs_.R[5]; break;
    }
  } else {
    switch (region) {
    case 0: bank = regs_.R[2]; break;
    case 1: bank = regs_.R[3]; break;
    case 2: bank = regs_.R[4]; break;
    case 3: bank = regs_.R[5]; break;
    case 4: bank = regs_.R[0]; break;
    case 5: bank = regs_.R[0] + 1; break;
    case 6: bank = regs_.R[1]; break;
    default: bank = regs_.R[1] + 1; break;
    }
  }
  int chr_addr = (bank << 10) + offset;
  assert(chr_addr < mem_.chr_rom_size);
  return chr_addr;
}

bool Mmc3::step_ppu() {
  bool curr_a12    = ppu_->addr_bus() & 0x1000;
  bool rising_edge = curr_a12 && !irq_.prev_a12;
  if (rising_edge) {
    if (ppu_->cycles() - irq_.prev_cycles >= cpu_to_ppu_cycles(3)) {
      clock_IRQ_counter();
    }
    irq_.prev_cycles = ppu_->cycles();
  }
  irq_.prev_a12 = curr_a12;
  return true;
}

void Mmc3::clock_IRQ_counter() {
  if (irq_.counter == 0 || irq_.reload) {
    irq_.counter = irq_.latch;
    irq_.reload  = false;
  } else {
    irq_.counter--;
  }
  if (irq_.counter == 0 && irq_.enabled) {
    cpu_->signal_IRQ();
  }
}
