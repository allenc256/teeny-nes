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

static constexpr int64_t APU_HZ    = 1789773;
static constexpr int64_t OUTPUT_HZ = 44100;

void Apu::power_on() {
  pulse_1_.power_on();
  pulse_2_.power_on();
  triangle_.power_on();
  noise_.power_on();
  out_.reset();
  fc_.mode        = false;
  fc_.irq_enabled = true;
  fc_.next_step   = 0;
  fc_.cycles_left = 0;
  cycles_         = 0;
  sample_counter_ = APU_HZ;
  low_pass_       = 0;
}

void Apu::reset() {
  pulse_1_.reset();
  pulse_2_.reset();
  triangle_.reset();
  noise_.reset();
  out_.reset();
  fc_.mode        = false;
  fc_.irq_enabled = true;
  fc_.next_step   = 0;
  fc_.cycles_left = 0;
  cycles_         = 0;
  sample_counter_ = APU_HZ;
  low_pass_       = 0;
}

void Apu::write_4000(uint8_t x) { pulse_1_.write_R0(x); }
void Apu::write_4001(uint8_t x) { pulse_1_.write_R1(x); }
void Apu::write_4002(uint8_t x) { pulse_1_.write_R2(x); }
void Apu::write_4003(uint8_t x) { pulse_1_.write_R3(x); }

void Apu::write_4004(uint8_t x) { pulse_2_.write_R0(x); }
void Apu::write_4005(uint8_t x) { pulse_2_.write_R1(x); }
void Apu::write_4006(uint8_t x) { pulse_2_.write_R2(x); }
void Apu::write_4007(uint8_t x) { pulse_2_.write_R3(x); }

void Apu::write_4008(uint8_t x) { triangle_.write_4008(x); }
void Apu::write_400A(uint8_t x) { triangle_.write_400A(x); }
void Apu::write_400B(uint8_t x) { triangle_.write_400B(x); }

void Apu::write_400C(uint8_t x) { noise_.write_400C(x); }
void Apu::write_400E(uint8_t x) { noise_.write_400E(x); }
void Apu::write_400F(uint8_t x) { noise_.write_400F(x); }

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
  pulse_2_.set_enabled(get_bit<1>(x));
  triangle_.set_enabled(get_bit<2>(x));
  noise_.set_enabled(get_bit<3>(x));
  // TODO: other channels & DMC
}

uint8_t Apu::read_4015() {
  uint8_t output = 0;

  if (pulse_1_.length_counter() != 0) {
    output |= bit(0);
  }
  if (pulse_2_.length_counter() != 0) {
    output |= bit(1);
  }
  if (triangle_.length_counter() != 0) {
    output |= bit(2);
  }
  if (noise_.length_counter() != 0) {
    output |= bit(3);
  }
  if (cpu_->pending_IRQ(Cpu::IrqSource::APU_FRAME_COUNTER)) {
    output |= bit(6);
  }

  cpu_->clear_IRQ(Cpu::IrqSource::APU_FRAME_COUNTER);

  return output;
}

void Apu::step() {
  triangle_.step();
  if (cycles_ & 1) {
    pulse_1_.step();
    pulse_2_.step();
    noise_.step();
  }

  step_frame_counter();

  cycles_++;

  sample_counter_ -= OUTPUT_HZ;
  if (sample_counter_ <= 0) {
    float mixed_output = triangle_.output() * 0.00851f +
                         (pulse_1_.output() + pulse_2_.output()) * 0.00752f +
                         noise_.output() * 0.00494f;
    low_pass_ = mixed_output * 0.7f + low_pass_ * 0.3f;
    out_.write(low_pass_);
    sample_counter_ += APU_HZ;
  }
}

void Apu::step_frame_counter() {
  if (fc_.cycles_left > 0) {
    fc_.cycles_left--;
    return;
  }

  bool should_clock_hf, should_signal_IRQ, should_reset_next_step;
  if (!fc_.mode) {
    should_clock_hf        = fc_.next_step & 1;
    should_signal_IRQ      = fc_.next_step == 3;
    should_reset_next_step = fc_.next_step == 3;
  } else {
    should_clock_hf        = fc_.next_step == 1 || fc_.next_step == 4;
    should_signal_IRQ      = false;
    should_reset_next_step = fc_.next_step == 4;
  }

  clock_quarter_frame();
  if (should_clock_hf) {
    clock_half_frame();
  }
  if (should_signal_IRQ) {
    cpu_->signal_IRQ(Cpu::IrqSource::APU_FRAME_COUNTER);
  }
  if (should_reset_next_step) {
    fc_.next_step = 0;
  } else {
    fc_.next_step++;
  }

  fc_.cycles_left = get_fc_cycles_left(fc_.next_step, fc_.mode);
}

void Apu::clock_quarter_frame() {
  pulse_1_.clock_quarter_frame();
  pulse_2_.clock_quarter_frame();
  triangle_.clock_quarter_frame();
  noise_.clock_quarter_frame();
  // TODO: other channels
}

void Apu::clock_half_frame() {
  pulse_1_.clock_half_frame();
  pulse_2_.clock_half_frame();
  triangle_.clock_half_frame();
  noise_.clock_half_frame();
  // TODO: other channels
}

static constexpr uint8_t DUTY_CYCLES[] = {
    0b01000000, 0b01100000, 0b01111000, 0b10011111
};

void ApuPulse::power_on() { reset(); }

void ApuPulse::reset() {
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

void ApuPulse::set_enabled(bool enabled) {
  enabled_ = enabled;
  if (!enabled) {
    length_counter_ = 0;
  }
}

void ApuPulse::write_R0(uint8_t x) {
  duty_cycle_     = DUTY_CYCLES[get_bits<6, 7>(x)];
  decay_loop_     = get_bit<5>(x);
  length_enabled_ = !get_bit<5>(x);
  decay_enabled_  = !get_bit<4>(x);
  decay_vol_      = get_bits<0, 3>(x);
}

void ApuPulse::write_R1(uint8_t x) {
  sweep_timer_   = get_bits<4, 6>(x);
  sweep_negate_  = get_bit<3>(x);
  sweep_shift_   = get_bits<0, 2>(x);
  sweep_reload_  = true;
  sweep_enabled_ = get_bit<7>(x) && sweep_shift_ != 0;
}

void ApuPulse::write_R2(uint8_t x) { freq_timer_ = (freq_timer_ & 0xff00) | x; }

static constexpr uint8_t LENGTH_TABLE[] = {10,  254, 20, 2,  40, 4,  80, 6,
                                           160, 8,   60, 10, 14, 12, 26, 14,
                                           12,  16,  24, 18, 48, 20, 96, 22,
                                           192, 24,  72, 26, 16, 28, 32, 30};

void ApuPulse::write_R3(uint8_t x) {
  freq_timer_ = (uint16_t)((freq_timer_ & 0xff) | (get_bits<0, 2>(x) << 8));

  if (enabled_) {
    length_counter_ = LENGTH_TABLE[get_bits<3, 7>(x)];
  }

  freq_counter_     = freq_timer_;
  duty_bit_         = 0x80;
  decay_reset_flag_ = true;
}

void ApuPulse::step() {
  if (freq_counter_ > 0) {
    freq_counter_--;
  } else {
    freq_counter_ = freq_timer_;
    duty_bit_     = std::rotr(duty_bit_, 1);
  }
}

#include <format>

void ApuPulse::clock_quarter_frame() {
  if (decay_reset_flag_) {
    decay_reset_flag_ = false;
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

void ApuPulse::clock_half_frame() {
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

uint8_t ApuPulse::output() {
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

bool ApuPulse::is_sweep_forcing_silence() {
  if (freq_timer_ < 8) {
    return true;
  } else if (!sweep_negate_ &&
             freq_timer_ + (freq_timer_ >> sweep_shift_) >= 0x800) {
    return true;
  } else {
    return false;
  }
}

void ApuTriangle::power_on() { reset(); }

void ApuTriangle::reset() {
  enabled_        = false;
  tri_step_       = 0;
  length_enabled_ = false;
  length_counter_ = 0;
  linear_control_ = false;
  linear_reload_  = false;
  linear_counter_ = 0;
  linear_load_    = 0;
  freq_counter_   = 0;
  freq_timer_     = 0;
}

void ApuTriangle::step() {
  bool ultrasonic = freq_timer_ < 2 && freq_counter_ == 0;
  bool clock_triunit;
  if (length_counter_ == 0 || linear_counter_ == 0 || ultrasonic) {
    clock_triunit = false;
  } else {
    clock_triunit = true;
  }

  if (clock_triunit) {
    if (freq_counter_ > 0) {
      freq_counter_--;
    } else {
      freq_counter_ = freq_timer_;
      tri_step_     = (tri_step_ + 1) & 0x1f;
    }
  }
}

void ApuTriangle::clock_quarter_frame() {
  if (linear_reload_) {
    linear_counter_ = linear_load_;
  } else if (linear_counter_ > 0) {
    linear_counter_--;
  }

  if (!linear_control_) {
    linear_reload_ = false;
  }
}

void ApuTriangle::clock_half_frame() {
  if (length_enabled_ && length_counter_ > 0) {
    length_counter_--;
  }
}

float ApuTriangle::output() {
  bool ultrasonic = freq_timer_ < 2 && freq_counter_ == 0;
  if (ultrasonic) {
    return 7.5;
  } else if (tri_step_ & 0x10) {
    return tri_step_ ^ 0x1f;
  } else {
    return tri_step_;
  }
}

void ApuTriangle::set_enabled(bool enabled) {
  enabled_ = enabled;
  if (!enabled) {
    length_counter_ = 0;
  }
}

void ApuTriangle::write_4008(uint8_t x) {
  linear_control_ = get_bit<7>(x);
  length_enabled_ = !linear_control_;
  linear_load_    = get_bits<0, 6>(x);
}

void ApuTriangle::write_400A(uint8_t x) {
  freq_timer_ = (freq_timer_ & 0xff00) | x;
}

void ApuTriangle::write_400B(uint8_t x) {
  freq_timer_ = (uint16_t)((freq_timer_ & 0xff) | (get_bits<0, 2>(x) << 8));

  if (enabled_) {
    length_counter_ = LENGTH_TABLE[get_bits<3, 7>(x)];
  }

  linear_reload_ = true;
}

void ApuNoise::power_on() { reset(); }

void ApuNoise::reset() {
  enabled_          = false;
  length_counter_   = 0;
  length_enabled_   = false;
  decay_loop_       = false;
  decay_enabled_    = false;
  decay_reset_flag_ = false;
  decay_counter_    = 0;
  decay_hidden_vol_ = 0;
  decay_vol_        = 0;
  freq_counter_     = 0;
  freq_timer_       = 0;
  shift_mode_       = false;
  noise_shift_      = 1;
}

void ApuNoise::step() {
  if (freq_counter_ > 0) {
    freq_counter_--;
  } else {
    freq_counter_ = freq_timer_;
    uint16_t bit;
    if (shift_mode_) {
      bit = ((noise_shift_ >> 6) & 1) ^ (noise_shift_ & 1);
    } else {
      bit = ((noise_shift_ >> 1) & 1) ^ (noise_shift_ & 1);
    }
    noise_shift_ = (uint16_t)((noise_shift_ & 0x7fff) | (bit << 15));
    noise_shift_ >>= 1;
  }
}

void ApuNoise::clock_quarter_frame() {
  if (decay_reset_flag_) {
    decay_reset_flag_ = false;
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

void ApuNoise::clock_half_frame() {
  if (length_enabled_ && length_counter_ > 0) {
    length_counter_--;
  }
}

uint8_t ApuNoise::output() {
  if (!(noise_shift_ & 1) && length_counter_ != 0) {
    if (decay_enabled_) {
      return decay_hidden_vol_;
    } else {
      return decay_vol_;
    }
  } else {
    return 0;
  }
}

void ApuNoise::set_enabled(bool enabled) {
  enabled_ = enabled;
  if (!enabled) {
    length_counter_ = 0;
  }
}

void ApuNoise::write_400C(uint8_t x) {
  decay_loop_     = get_bit<5>(x);
  length_enabled_ = !get_bit<5>(x);
  decay_enabled_  = !get_bit<4>(x);
  decay_vol_      = get_bits<0, 3>(x);
}

static constexpr uint16_t NOISE_FREQ_TABLE[] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

void ApuNoise::write_400E(uint8_t x) {
  freq_timer_ = NOISE_FREQ_TABLE[get_bits<0, 3>(x)] >> 1;
  shift_mode_ = get_bit<7>(x);
}

void ApuNoise::write_400F(uint8_t x) {
  if (enabled_) {
    length_counter_ = LENGTH_TABLE[get_bits<3, 7>(x)];
  }
  decay_reset_flag_ = true;
}

void OutputBuffer::reset() {
  written_ = 0;
  read_    = 0;
}

int OutputBuffer::available() const {
  return (int)std::min(written_ - read_, (int64_t)CAPACITY);
}

void OutputBuffer::write(float sample) {
  static_assert(std::has_single_bit(CAPACITY));
  buffer_[written_ & (CAPACITY - 1)] = sample;
  written_++;
}

float OutputBuffer::read() {
  if (written_ - read_ > (int64_t)CAPACITY) {
    read_ = written_ - CAPACITY;
  }
  float result = buffer_[read_ & (CAPACITY - 1)];
  read_++;
  return result;
}
