#include <bit>
#include <cassert>

#include "src/emu/apu.h"
#include "src/emu/cpu.h"

// References:
// Forum thread #1: https://forums.nesdev.org/viewtopic.php?f=3&t=13749
// Forum thread #2: https://forums.nesdev.org/viewtopic.php?f=3&t=13767

static constexpr uint8_t bit(int index) { return (uint8_t)(1 << index); }

static constexpr uint8_t bits(int start_index, int end_index) {
  uint8_t result = 0;
  for (int i = start_index; i <= end_index; i++) {
    result |= bit(i);
  }
  return result;
}

template <int index> static constexpr bool get_bit(uint8_t x) {
  return x & bit(index);
}

template <int start_index, int end_index>
static constexpr uint8_t get_bits(uint8_t x) {
  return (x & bits(start_index, end_index)) >> start_index;
}

void Apu::power_on() {
  pulse_1_.power_on();
  fc_.mode        = false;
  fc_.irq_enabled = true;
  fc_.next_step   = 0;
  fc_.cycles_left = 0;
  cycles_         = 0;
}

void Apu::reset() {
  pulse_1_.reset();
  fc_.mode        = false;
  fc_.irq_enabled = true;
  fc_.next_step   = 0;
  fc_.cycles_left = 0;
  cycles_         = 0;
}

void Apu::write_4000(uint8_t x) { pulse_1_.write_R0(x); }
void Apu::write_4001(uint8_t x) { pulse_1_.write_R1(x); }
void Apu::write_4002(uint8_t x) { pulse_1_.write_R2(x); }
void Apu::write_4003(uint8_t x) { pulse_1_.write_R3(x); }

static int get_fc_cycles_left(int next_step, bool mode) {
  if (!mode) {
    switch (next_step) {
    case 0: return 7457;
    case 1: return 7456;
    case 2: return 7458;
    case 3: return 7459;
    default: throw std::runtime_error("unreachable");
    }
  } else {
    switch (next_step) {
    case 0: return 7457;
    case 1: return 7456;
    case 2: return 7458;
    case 3: return 7458;
    case 4: return 7453;
    default: throw std::runtime_error("unreachable");
    }
  }
}

void Apu::write_4017(uint8_t x) {
  fc_.mode        = get_bit<7>(x);
  fc_.irq_enabled = !get_bit<6>(x);
  fc_.next_step   = 0;
  fc_.cycles_left = get_fc_cycles_left(fc_.next_step, fc_.mode);

  // From nesdev:
  //    Writing to $4017 with bit 7 set ($80) will immediately clock all of its
  //    controlled units at the beginning of the 5-step sequence; with bit 7
  //    clear, only the sequence is reset without clocking any of its units.
  if (fc_.mode) {
    clock_quarter_frame();
    clock_half_frame();
  }

  if (!fc_.irq_enabled) {
    cpu_->clear_IRQ(Cpu::IrqSource::APU_FRAME_COUNTER);
  }
}

void Apu::write_4015(uint8_t x) {
  pulse_1_.set_enabled(get_bit<0>(x));
  // TODO: other channels & DMC
}

uint8_t Apu::read_4015() {
  uint8_t output = 0;

  if (pulse_1_.length_counter() != 0) {
    output |= bit(0);
  }

  if (cpu_->pending_IRQ(Cpu::IrqSource::APU_FRAME_COUNTER)) {
    output |= bit(6);
  }

  cpu_->clear_IRQ(Cpu::IrqSource::APU_FRAME_COUNTER);

  return output;
}

void Apu::step() {
  if (cycles_ & 1) {
    pulse_1_.step();
  }

  step_frame_counter();

  cycles_++;
}

void Apu::step_frame_counter() {
  if (fc_.cycles_left > 0) {
    fc_.cycles_left--;
    return;
  }

  bool should_clock_hf, should_signal_IRQ, should_reset;
  if (!fc_.mode) {
    should_clock_hf   = fc_.next_step & 1;
    should_signal_IRQ = fc_.next_step == 3;
    should_reset      = fc_.next_step == 3;
  } else {
    should_clock_hf   = fc_.next_step == 1 || fc_.next_step == 4;
    should_signal_IRQ = false;
    should_reset      = fc_.next_step == 4;
  }

  clock_quarter_frame();
  if (should_clock_hf) {
    clock_half_frame();
  }
  if (should_signal_IRQ) {
    cpu_->signal_IRQ(Cpu::IrqSource::APU_FRAME_COUNTER);
  }

  if (should_reset) {
    fc_.next_step = 0;
  } else {
    fc_.next_step++;
  }

  fc_.cycles_left = get_fc_cycles_left(fc_.next_step, fc_.mode);
}

float Apu::output() { return pulse_1_.output() / 15.0f; }

void Apu::clock_quarter_frame() {
  pulse_1_.clock_quarter_frame();
  // TODO: other channels
}

void Apu::clock_half_frame() {
  pulse_1_.clock_half_frame();
  // TODO: other channels
}

static constexpr uint8_t DUTY_CYCLES[] = {
    0b01000000, 0b01100000, 0b01111000, 0b10011111
};

void PulseWave::power_on() { reset(); }

void PulseWave::reset() {
  enabled_          = false;
  duty_cycle_       = 0;
  duty_bit_         = 0;
  length_counter_   = 0;
  length_enabled_   = false;
  decay_loop_       = false;
  decay_enabled_    = false;
  decay_reset_flag_ = false;
  decay_counter_    = 0;
  decay_hidden_vol_ = 0;
  decay_vol_        = 0;
  sweep_counter_    = 0;
  sweep_timer_      = 0;
  sweep_negate_     = false;
  sweep_shift_      = 0;
  sweep_reload_     = false;
  sweep_enabled_    = false;
  freq_counter_     = 0;
  freq_timer_       = 0;
}

void PulseWave::set_enabled(bool enabled) {
  enabled_ = enabled;
  if (!enabled) {
    length_counter_ = 0;
  }
}

void PulseWave::write_R0(uint8_t x) {
  duty_cycle_     = DUTY_CYCLES[get_bits<6, 7>(x)];
  decay_loop_     = get_bit<5>(x);
  length_enabled_ = !get_bit<5>(x);
  decay_enabled_  = !get_bit<4>(x);
  decay_vol_      = get_bits<0, 3>(x);
}

void PulseWave::write_R1(uint8_t x) {
  sweep_timer_   = get_bits<4, 6>(x);
  sweep_negate_  = get_bit<3>(x);
  sweep_shift_   = get_bits<0, 2>(x);
  sweep_reload_  = true;
  sweep_enabled_ = get_bit<7>(x) && sweep_shift_ != 0;
}

void PulseWave::write_R2(uint8_t x) {
  freq_timer_ = (freq_timer_ & 0xff00) | x;
}

static constexpr uint8_t LENGTH_TABLE[] = {10,  254, 20, 2,  40, 4,  80, 6,
                                           160, 8,   60, 10, 14, 12, 26, 14,
                                           12,  16,  24, 18, 48, 20, 96, 22,
                                           192, 24,  72, 26, 16, 28, 32, 30};

void PulseWave::write_R3(uint8_t x) {
  freq_timer_ = (uint16_t)((freq_timer_ & 0xff) | (get_bits<0, 3>(x) << 8));

  if (enabled_) {
    length_counter_ = LENGTH_TABLE[get_bits<3, 7>(x)];
  }

  freq_counter_     = freq_timer_;
  duty_bit_         = 0x80;
  decay_reset_flag_ = true;
}

void PulseWave::step() {
  if (freq_counter_ > 0) {
    freq_counter_--;
  } else {
    freq_counter_ = freq_timer_;
    duty_bit_     = std::rotr(duty_bit_, 1);
  }
}

void PulseWave::clock_quarter_frame() {
  if (decay_reset_flag_) {
    decay_reset_flag_ = true;
    decay_hidden_vol_ = 0xf;
    decay_counter_    = decay_vol_;
  } else {
    if (decay_counter_ > 0) {
      decay_counter_--;
    } else {
      decay_counter_ = decay_vol_;
      if (decay_hidden_vol_ > 0) {
        decay_hidden_vol_--;
      } else if (decay_loop_) {
        decay_hidden_vol_ = 0xf;
      }
    }
  }
}

void PulseWave::clock_half_frame() {
  if (sweep_reload_) {
    sweep_counter_ = sweep_timer_;
    // TODO: handle edge case here for APU sweep
    sweep_reload_ = false;
  } else if (sweep_counter_ > 0) {
    sweep_counter_--;
  } else {
    sweep_counter_ = sweep_timer_;
    if (sweep_enabled_ && !is_sweep_forcing_silence()) {
      if (sweep_negate_) {
        freq_timer_ -= (freq_timer_ >> sweep_shift_) + is_pulse_1_;
      } else {
        freq_timer_ += (freq_timer_ >> sweep_shift_);
      }
    }
  }

  if (length_enabled_ && length_counter_ > 0) {
    length_counter_--;
  }
}

uint8_t PulseWave::output() {
  if ((duty_cycle_ & duty_bit_) && length_counter_ != 0 &&
      !is_sweep_forcing_silence()) {
    if (decay_enabled_) {
      return decay_hidden_vol_;
    } else {
      return decay_vol_;
    }
  } else {
    return 0;
  }
}

bool PulseWave::is_sweep_forcing_silence() {
  if (freq_timer_ < 0) {
    return true;
  } else if (!sweep_negate_ &&
             freq_timer_ + (freq_timer_ >> sweep_shift_) >= 0x800) {
    return true;
  } else {
    return false;
  }
}
