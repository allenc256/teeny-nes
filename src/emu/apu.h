#pragma once

#include <cstdint>

class Cpu;

class PulseWave {
public:
  PulseWave(bool is_pulse_1) : is_pulse_1_(is_pulse_1) {}

  void    power_on();
  void    reset();
  void    step();
  void    clock_quarter_frame();
  void    clock_half_frame();
  uint8_t output();

  bool    enabled() const { return enabled_; }
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

class OutputBuffer {
public:
  static constexpr size_t CAPACITY = 2048;

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
  void set_cpu(Cpu *cpu) { cpu_ = cpu; }

  void power_on();
  void reset();
  void step();

  OutputBuffer &output() { return out_; }
  int64_t       cycles() { return cycles_; }

  void write_4000(uint8_t x);
  void write_4001(uint8_t x);
  void write_4002(uint8_t x);
  void write_4003(uint8_t x);

  void write_4004(uint8_t x);
  void write_4005(uint8_t x);
  void write_4006(uint8_t x);
  void write_4007(uint8_t x);

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
  PulseWave    pulse_1_ = {true};
  PulseWave    pulse_2_ = {false};
  FrameCounter fc_;
  OutputBuffer out_;
  int64_t      cycles_;
  int64_t      sample_counter_;
};
