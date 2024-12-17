#pragma once

#include <coroutine>

class Coroutine {
public:
  class promise_type {
  public:
    Coroutine get_return_object() {
      return Coroutine(std::coroutine_handle<promise_type>::from_promise(*this)
      );
    }

    std::suspend_always initial_suspend() { return {}; }
    void                return_void() {}
    void                unhandled_exception() {}
    std::suspend_always final_suspend() noexcept { return {}; }
  };

  Coroutine()                                            = delete;
  Coroutine(Coroutine &&other) noexcept                  = delete;
  Coroutine(const Coroutine &other)                      = delete;
  Coroutine &operator=(const Coroutine &other)           = delete;
  Coroutine &operator=(const Coroutine &&other) noexcept = delete;

  Coroutine(std::coroutine_handle<promise_type> handle) : handle_(handle) {}

  ~Coroutine() {
    if (handle_) {
      handle_.destroy();
    }
  }

  void step() { handle_.resume(); }

private:
  std::coroutine_handle<promise_type> handle_;
};
