#include <bit>
#include <cassert>

#include "src/emu/apu.h"
#include "src/emu/cpu.h"

// References:
// Forum thread #1: https://forums.nesdev.org/viewtopic.php?f=3&t=13749
// Forum thread #2: https://forums.nesdev.org/viewtopic.php?f=3&t=13767

static constexpr float MIXER_PULSE_LUT[] = {
    0,
    0.011609139523578026,
    0.022939481268011527,
    0.03400094921689606,
    0.04480300187617261,
    0.05535465924895688,
    0.06566452795600367,
    0.07574082464884459,
    0.08559139784946236,
    0.09522374833850243,
    0.10464504820333041,
    0.11386215864759427,
    0.12288164665523155,
    0.13170980059397538,
    0.14035264483627205,
    0.1488159534690486,
    0.15710526315789472,
    0.16522588522588522,
    0.1731829170024174,
    0.18098125249301955,
    0.18862559241706162,
    0.19612045365662886,
    0.20347017815646784,
    0.21067894131185272,
    0.21775075987841944,
    0.2246894994354535,
    0.2314988814317673,
    0.23818248984115256,
    0.2447437774524158,
    0.2511860718171926,
    0.25751258087706685,
};

static constexpr float MIXER_TND_LUT[] = {
    0,
    0.006699823979696262,
    0.01334502018019487,
    0.01993625400950099,
    0.026474180112418616,
    0.032959442587297105,
    0.03939267519756107,
    0.04577450157816932,
    0.05210553543714433,
    0.05838638075230885,
    0.06461763196336215,
    0.07079987415942428,
    0.07693368326217241,
    0.08301962620468999,
    0.08905826110614481,
    0.09505013744240969,
    0.10099579621273477,
    0.10689577010257789,
    0.11275058364269584,
    0.11856075336459644,
    0.12432678795244785,
    0.1300491883915396,
    0.13572844811338536,
    0.1413650531375568,
    0.1469594822103333,
    0.15251220694025122,
    0.15802369193063237,
    0.16349439490917161,
    0.16892476685465738,
    0.1743152521209005,
    0.1796662885579421,
    0.18497830763060993,
    0.19025173453449087,
    0.19548698830938505,
    0.20068448195030472,
    0.20584462251608032,
    0.2109678112356332,
    0.2160544436119733,
    0.2211049095239788,
    0.22611959332601225,
    0.2310988739454269,
    0.23604312497801538,
    0.24095271478145042,
    0.24582800656676793,
    0.25066935848793903,
    0.25547712372957787,
    0.2602516505928307,
    0.26499328257948945,
    0.26970235847437257,
    0.27437921242601526,
    0.27902417402570834,
    0.28363756838492643,
    0.2882197162111822,
    0.292770933882345,
    0.29729153351945914,
    0.3017818230580978,
    0.3062421063182866,
    0.31067268307302937,
    0.31507384911547015,
    0.3194458963247213,
    0.32378911273039,
    0.3281037825758322,
    0.3323901863801631,
    0.33664860099905314,
    0.3408792996843372,
    0.34508255214246325,
    0.349258624591807,
    0.3534077798188791,
    0.3575302772334479,
    0.36162637292260397,
    0.3656963197037888,
    0.3697403671768112,
    0.3737587617748739,
    0.37775174681463214,
    0.38171956254530554,
    0.38566244619686446,
    0.3895806320273106,
    0.3934743513690717,
    0.3973438326745308,
    0.40118930156070615,
    0.405010980853104,
    0.4088090906287582,
    0.41258384825847705,
    0.4163354684483128,
    0.42006416328027124,
    0.4237701422522769,
    0.42745361231741014,
    0.4311147779224318,
    0.4347538410456096,
    0.43837100123386197,
    0.4419664556392331,
    0.44554039905471293,
    0.44909302394941686,
    0.4526245205031371,
    0.45613507664027986,
    0.4596248780632002,
    0.4630941082849479,
    0.4665429486614358,
    0.46997157842304194,
    0.47338017470565896,
    0.4767689125811996,
    0.48013796508757145,
    0.48348750325813084,
    0.48681769615062515,
    0.49012871087563703,
    0.493420712624537,
    0.49669386469695664,
    0.49994832852779125,
    0.5031842637137408,
    0.5064018280393993,
    0.5096011775029012,
    0.5127824663411329,
    0.5159458470545188,
    0.5190914704313901,
    0.5222194855719443,
    0.5253300399118033,
    0.528423279245178,
    0.5314993477476477,
    0.5345583879985607,
    0.5376005410030638,
    0.5406259462137686,
    0.5436347415520602,
    0.5466270634290563,
    0.5496030467662235,
    0.5525628250156552,
    0.5555065301800212,
    0.5584342928321915,
    0.5613462421345432,
    0.5642425058579547,
    0.5671232104004943,
    0.5699884808058077,
    0.5728384407812124,
    0.5756732127155,
    0.5784929176964575,
    0.5812976755281083,
    0.5840876047476803,
    0.5868628226423054,
    0.5896234452654553,
    0.5923695874531196,
    0.595101362839729,
    0.5978188838738291,
    0.6005222618335111,
    0.6032116068415997,
    0.6058870278806079,
    0.6085486328074569,
    0.6111965283679723,
    0.6138308202111536,
    0.6164516129032258,
    0.6190590099414757,
    0.6216531137678758,
    0.6242340257825014,
    0.6268018463567424,
    0.6293566748463153,
    0.6318986096040777,
    0.6344277479926501,
    0.6369441863968464,
    0.6394480202359187,
    0.6419393439756177,
    0.6444182511400732,
    0.6468848343234979,
    0.6493391852017159,
    0.6517813945435207,
    0.6542115522218658,
    0.6566297472248885,
    0.659036067666773,
    0.6614306007984521,
    0.6638134330181533,
    0.6661846498817908,
    0.6685443361132047,
    0.670892575614252,
    0.6732294514747513,
    0.6755550459822829,
    0.6778694406318475,
    0.6801727161353863,
    0.6824649524311629,
    0.684746228693012,
    0.6870166233394548,
    0.6892762140426848,
    0.6915250777374256,
    0.693763290629662,
    0.6959909282052493,
    0.6982080652383982,
    0.7004147758000423,
    0.7026111332660865,
    0.70479721032554,
    0.7069730789885358,
    0.7091388105942369,
    0.7112944758186339,
    0.7134401446822323,
    0.7155758865576349,
    0.7177017701770176,
    0.7198178636395035,
    0.7219242344184336,
    0.7240209493685391,
    0.7261080747330146,
    0.7281856761504939,
    0.7302538186619317,
    0.7323125667173908,
    0.734361984182737,
    0.7364021343462434,
    0.7384330799251054,
    0.7404548830718675,
    0.742467605380763,
};

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

static constexpr int64_t APU_HZ = 1789773;

void Apu::set_cpu(Cpu *cpu) {
  cpu_ = cpu;
  dmc_.set_cpu(cpu);
  fc_.set_cpu(cpu);
}

void Apu::power_on() {
  pulse_1_.power_on();
  pulse_2_.power_on();
  triangle_.power_on();
  noise_.power_on();
  dmc_.power_on();
  fc_.power_on();
  out_.reset();
  cycles_         = 0;
  sample_counter_ = APU_HZ;
  output_ema_     = 0;
}

void Apu::reset() {
  pulse_1_.reset();
  pulse_2_.reset();
  triangle_.reset();
  noise_.reset();
  dmc_.reset();
  fc_.reset();
  out_.reset();
  cycles_         = 0;
  sample_counter_ = APU_HZ;
  output_ema_     = 0;
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

void Apu::write_4010(uint8_t x) { dmc_.write_4010(x); }
void Apu::write_4011(uint8_t x) { dmc_.write_4011(x); }
void Apu::write_4012(uint8_t x) { dmc_.write_4012(x); }
void Apu::write_4013(uint8_t x) { dmc_.write_4013(x); }

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
  auto clock = fc_.write_4017(x);
  clock_frame_counter(clock);
}

void Apu::write_4015(uint8_t x) {
  pulse_1_.set_enabled(get_bit<0>(x));
  pulse_2_.set_enabled(get_bit<1>(x));
  triangle_.set_enabled(get_bit<2>(x));
  noise_.set_enabled(get_bit<3>(x));
  dmc_.set_enabled(get_bit<4>(x));
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
  if (dmc_.length_counter() != 0) {
    output |= bit(4);
  }
  if (cpu_->pending_IRQ(Cpu::IrqSource::APU_FRAME_COUNTER)) {
    output |= bit(6);
  }
  if (cpu_->pending_IRQ(Cpu::IrqSource::APU_DMC)) {
    output |= bit(7);
  }

  cpu_->clear_IRQ(Cpu::IrqSource::APU_FRAME_COUNTER);

  return output;
}

void Apu::step() {
  triangle_.step();
  dmc_.step();
  if (cycles_ & 1) {
    pulse_1_.step();
    pulse_2_.step();
    noise_.step();
  }

  auto clock = fc_.step();
  clock_frame_counter(clock);

  cycles_++;

  // Reference: https://www.nesdev.org/wiki/APU_Mixer#Lookup_Table
  uint8_t pulse1   = pulse_1_.output();
  uint8_t pulse2   = pulse_2_.output();
  uint8_t triangle = triangle_.output(cycles_ & 1);
  uint8_t noise    = noise_.output();
  uint8_t dmc      = dmc_.output();
  assert(pulse1 < 16 && pulse2 < 16 && triangle < 16 && noise < 16);
  assert(dmc < 128);
  float output = MIXER_PULSE_LUT[pulse1 + pulse2] +
                 MIXER_TND_LUT[triangle * 3 + noise * 2 + dmc];
  assert(output >= 0 && output <= 1);
  output_ema_ = output * 0.05f + output_ema_ * 0.95f;

  sample_counter_ -= sample_rate_;
  if (sample_counter_ <= 0) {
    out_.write(output_ema_);
    sample_counter_ += APU_HZ;
  }
}

void Apu::clock_frame_counter(ApuFrameCounter::Clock clock) {
  if (clock.quarter_frame) {
    pulse_1_.clock_quarter_frame();
    pulse_2_.clock_quarter_frame();
    triangle_.clock_quarter_frame();
    noise_.clock_quarter_frame();
  }
  if (clock.half_frame) {
    pulse_1_.clock_half_frame();
    pulse_2_.clock_half_frame();
    triangle_.clock_half_frame();
    noise_.clock_half_frame();
  }
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

uint8_t ApuTriangle::output(bool odd_cycle) {
  bool ultrasonic = freq_timer_ < 2 && freq_counter_ == 0;
  if (ultrasonic) {
    return 7 + odd_cycle;
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

void ApuDmc::power_on() { reset(); }

void ApuDmc::reset() {
  irq_enabled_   = false;
  loop_          = false;
  output_        = 0;
  output_shift_  = 0;
  output_silent_ = false;
  output_bits_   = 0;
  sample_buffer_ = 0;
  sample_empty_  = false;
  addr_          = 0;
  addr_load_     = 0;
  length_        = 0;
  length_load_   = 0;
  freq_timer_    = 0;
  freq_counter_  = 0;
}

void ApuDmc::set_enabled(bool enabled) {
  if (enabled) {
    if (length_ == 0) {
      length_ = length_load_;
      addr_   = addr_load_;
    }
  } else {
    length_ = 0;
  }

  cpu_->clear_IRQ(Cpu::APU_DMC);
}

static constexpr uint16_t DMC_FREQ_TABLE[] = {
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54
};

void ApuDmc::write_4010(uint8_t x) {
  irq_enabled_ = get_bit<7>(x);
  loop_        = get_bit<6>(x);
  freq_timer_  = DMC_FREQ_TABLE[get_bits<0, 3>(x)];

  if (!irq_enabled_) {
    cpu_->clear_IRQ(Cpu::APU_DMC);
  }
}

void ApuDmc::write_4011(uint8_t x) { output_ = get_bits<0, 6>(x); }

void ApuDmc::write_4012(uint8_t x) {
  addr_load_ = (uint16_t)(0xc000 | (x << 6));
}

void ApuDmc::write_4013(uint8_t x) { length_load_ = (uint16_t)((x << 4) + 1); }

void ApuDmc::step() {
  if (freq_counter_ > 0) {
    freq_counter_--;
  } else {
    freq_counter_ = freq_timer_;
    if (!output_silent_) {
      if ((output_shift_ & 1) && output_ < 0x7e) {
        output_ += 2;
      } else if (!(output_shift_ & 1) && output_ > 0x01) {
        output_ -= 2;
      }
    }
    output_bits_--;
    output_shift_ >>= 1;
    if (output_bits_ == 0) {
      output_bits_   = 8;
      output_shift_  = sample_buffer_;
      output_silent_ = sample_empty_;
      sample_empty_  = true;
    }
  }

  if (length_ > 0 && sample_empty_) {
    sample_buffer_ = cpu_->peek(addr_);
    sample_empty_  = false;
    addr_          = (uint16_t)((addr_ + 1) | 0x8000);
    length_--;

    if (length_ == 0) {
      if (loop_) {
        length_ = length_load_;
        addr_   = addr_load_;
      } else if (irq_enabled_) {
        cpu_->signal_IRQ(Cpu::APU_DMC);
      }
    }
  }
}

void ApuFrameCounter::power_on() { reset(); }

void ApuFrameCounter::reset() {
  mode_        = false;
  irq_enabled_ = true;
  next_step_   = 0;
  cycles_left_ = 0;
}

ApuFrameCounter::Clock ApuFrameCounter::write_4017(uint8_t x) {
  mode_        = get_bit<7>(x);
  irq_enabled_ = !get_bit<6>(x);
  next_step_   = 0;
  cycles_left_ = get_fc_cycles_left(next_step_, mode_);

  if (!irq_enabled_) {
    cpu_->clear_IRQ(Cpu::IrqSource::APU_FRAME_COUNTER);
  }

  if (mode_) {
    return {.quarter_frame = true, .half_frame = true};
  } else {
    return {.quarter_frame = false, .half_frame = false};
  }
}

ApuFrameCounter::Clock ApuFrameCounter::step() {
  if (cycles_left_ > 0) {
    cycles_left_--;
    return {.quarter_frame = false, .half_frame = false};
  }

  Clock clock;
  bool  should_signal_IRQ, should_reset_next_step;
  if (!mode_) {
    clock.half_frame       = next_step_ & 1;
    should_signal_IRQ      = irq_enabled_ && next_step_ == 3;
    should_reset_next_step = next_step_ == 3;
  } else {
    clock.half_frame       = next_step_ == 1 || next_step_ == 4;
    should_signal_IRQ      = false;
    should_reset_next_step = next_step_ == 4;
  }

  clock.quarter_frame = true;

  if (should_signal_IRQ) {
    cpu_->signal_IRQ(Cpu::IrqSource::APU_FRAME_COUNTER);
  }
  if (should_reset_next_step) {
    next_step_ = 0;
  } else {
    next_step_++;
  }

  cycles_left_ = get_fc_cycles_left(next_step_, mode_);

  return clock;
}

void ApuBuffer::reset() {
  written_ = 0;
  read_    = 0;
}

int ApuBuffer::available() const {
  return (int)std::min(written_ - read_, (int64_t)CAPACITY);
}

void ApuBuffer::write(float sample) {
  static_assert(std::has_single_bit(CAPACITY));
  buffer_[written_ & (CAPACITY - 1)] = sample;
  written_++;
}

float ApuBuffer::read() {
  if (written_ - read_ > (int64_t)CAPACITY) {
    read_ = written_ - CAPACITY;
  }
  float result = buffer_[read_ & (CAPACITY - 1)];
  read_++;
  return result;
}
