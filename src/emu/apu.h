#pragma once

#include <cstdint>

class Cpu;

class ApuPulse {
public:
  ApuPulse(bool is_pulse_1) : is_pulse_1_(is_pulse_1) {}

  void    power_on();
  void    reset();
  void    step();
  void    clock_quarter_frame();
  void    clock_half_frame();
  uint8_t output();

  uint8_t length_counter() const { return length_counter_; }
  void    set_enabled(bool enabled);

  void write_R0(uint8_t x);
  void write_R1(uint8_t x);
  void write_R2(uint8_t x);
  void write_R3(uint8_t x);

private:
  bool is_sweep_forcing_silence();

  bool enabled_;
  bool is_pulse_1_;

  uint8_t duty_cycle_;
  uint8_t duty_bit_;

  uint8_t length_counter_;
  bool    length_enabled_;

  bool    decay_loop_;
  bool    decay_enabled_;
  bool    decay_reset_flag_;
  uint8_t decay_counter_;
  uint8_t decay_hidden_vol_;
  uint8_t decay_vol_;

  uint8_t sweep_counter_;
  uint8_t sweep_timer_;
  bool    sweep_negate_;
  uint8_t sweep_shift_;
  bool    sweep_reload_;
  bool    sweep_enabled_;

  uint16_t freq_counter_;
  uint16_t freq_timer_;
};

class ApuTriangle {
public:
  void  power_on();
  void  reset();
  void  step();
  void  clock_quarter_frame();
  void  clock_half_frame();
  float output();

  uint8_t length_counter() const { return length_counter_; }
  void    set_enabled(bool enabled);

  void write_4008(uint8_t x);
  void write_400A(uint8_t x);
  void write_400B(uint8_t x);

private:
  bool enabled_;

  uint8_t tri_step_;

  bool    length_enabled_;
  uint8_t length_counter_;

  bool    linear_control_;
  bool    linear_reload_;
  uint8_t linear_counter_;
  uint8_t linear_load_;

  uint16_t freq_counter_;
  uint16_t freq_timer_;
};

class ApuNoise {
public:
  void    power_on();
  void    reset();
  void    step();
  void    clock_quarter_frame();
  void    clock_half_frame();
  uint8_t output();

  uint8_t length_counter() const { return length_counter_; }
  void    set_enabled(bool enabled);

  void write_400C(uint8_t x);
  void write_400E(uint8_t x);
  void write_400F(uint8_t x);

private:
  bool enabled_;

  uint8_t length_counter_;
  bool    length_enabled_;

  bool    decay_loop_;
  bool    decay_enabled_;
  bool    decay_reset_flag_;
  uint8_t decay_counter_;
  uint8_t decay_hidden_vol_;
  uint8_t decay_vol_;

  uint16_t freq_counter_;
  uint16_t freq_timer_;

  bool     shift_mode_;
  uint16_t noise_shift_;
};

class ApuDmc {
public:
  void set_cpu(Cpu *cpu) { cpu_ = cpu; }

  void    power_on();
  void    reset();
  void    step();
  uint8_t output() { return output_; }

  uint16_t length_counter() const { return length_; }
  void     set_enabled(bool enabled);

  void write_4010(uint8_t x);
  void write_4011(uint8_t x);
  void write_4012(uint8_t x);
  void write_4013(uint8_t x);

private:
  Cpu *cpu_ = nullptr;

  bool irq_enabled_;
  bool loop_;

  uint8_t output_;
  uint8_t output_shift_;
  bool    output_silent_;
  uint8_t output_bits_;
  uint8_t sample_buffer_;
  bool    sample_empty_;

  uint16_t addr_;
  uint16_t addr_load_;
  uint16_t length_;
  uint16_t length_load_;

  uint16_t freq_timer_;
  uint16_t freq_counter_;
};

class ApuBuffer {
public:
  static constexpr size_t CAPACITY = 1024;

  void  reset();
  int   available() const;
  void  write(float sample);
  float read();

private:
  float   buffer_[CAPACITY];
  int64_t written_;
  int64_t read_;
};

class Apu {
public:
  void set_cpu(Cpu *cpu);

  void power_on();
  void reset();
  void step();

  ApuBuffer &output() { return out_; }
  int64_t    cycles() { return cycles_; }

  void write_4000(uint8_t x);
  void write_4001(uint8_t x);
  void write_4002(uint8_t x);
  void write_4003(uint8_t x);

  void write_4004(uint8_t x);
  void write_4005(uint8_t x);
  void write_4006(uint8_t x);
  void write_4007(uint8_t x);

  void write_4008(uint8_t x);
  void write_400A(uint8_t x);
  void write_400B(uint8_t x);

  void write_400C(uint8_t x);
  void write_400E(uint8_t x);
  void write_400F(uint8_t x);

  void write_4010(uint8_t x);
  void write_4011(uint8_t x);
  void write_4012(uint8_t x);
  void write_4013(uint8_t x);

  void write_4015(uint8_t x);
  void write_4017(uint8_t x);

  uint8_t read_4015();

private:
  struct FrameCounter {
    bool mode;
    bool irq_enabled;
    int  next_step;
    int  cycles_left;
  };

  void step_frame_counter();
  void clock_quarter_frame();
  void clock_half_frame();

  Cpu         *cpu_     = nullptr;
  ApuPulse     pulse_1_ = {true};
  ApuPulse     pulse_2_ = {false};
  ApuTriangle  triangle_;
  ApuNoise     noise_;
  ApuDmc       dmc_;
  FrameCounter fc_;
  ApuBuffer    out_;
  float        output_ema_;
  int64_t      cycles_;
  int64_t      sample_counter_;
};
