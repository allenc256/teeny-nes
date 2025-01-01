#include <cassert>

#include "src/emu/nes.h"
#include "src/emu/timer.h"

Timer::Timer() { reset(); }

void Timer::reset() {
  timestamp_ = std::chrono::high_resolution_clock::now();
  remainder_ = 0;
}

static constexpr int64_t CPU_HZ            = 1789773;
static constexpr int64_t NANOS_PER_SEC     = 1000000000;
static constexpr int64_t MAX_CYCLES_TO_RUN = CPU_HZ / 20;

using Timestamp = std::chrono::high_resolution_clock::time_point;

static int64_t elapsed_nanos(Timestamp start, Timestamp now) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(now - start)
      .count();
}

void Timer::run(Nes &nes) {
  auto    now       = std::chrono::high_resolution_clock::now();
  int64_t elapsed   = elapsed_nanos(timestamp_, now);
  timestamp_        = now;
  int64_t numerator = elapsed * CPU_HZ;

  if (numerator <= remainder_) {
    remainder_ -= numerator;
    return;
  }

  numerator -= remainder_;
  int64_t min_cycles_to_run = numerator / NANOS_PER_SEC;
  if (min_cycles_to_run > MAX_CYCLES_TO_RUN) {
    min_cycles_to_run = MAX_CYCLES_TO_RUN;
    remainder_        = 0;
  } else {
    remainder_ = numerator % NANOS_PER_SEC;
  }

  int64_t target = nes.cpu().cycles() + min_cycles_to_run;
  while (nes.cpu().cycles() < target) {
    nes.step();
  }

  remainder_ += (nes.cpu().cycles() - target) * NANOS_PER_SEC;
}
