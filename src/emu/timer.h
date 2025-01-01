#pragma once

#include <chrono>

class Nes;

class Timer {
public:
  Timer();

  void reset();
  void run(Nes &nes);

private:
  using Timestamp = std::chrono::high_resolution_clock::time_point;

  Timestamp timestamp_;
  int64_t   remainder_;
};
