#pragma once

#include <coroutine>
#include <exception>

class Coroutine {
public:
  class promise_type {
  public:
    Coroutine get_return_object() {
      return Coroutine(std::coroutine_handle<promise_type>::from_promise(*this)
      );
    }

    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { exn_ = std::current_exception(); }

    void return_void() {
      if (exn_) {
        std::rethrow_exception(exn_);
      }
    }

  private:
    std::exception_ptr exn_;
  };

  Coroutine()                                  = delete;
  Coroutine(Coroutine &&other) noexcept        = delete;
  Coroutine(const Coroutine &other)            = delete;
  Coroutine &operator=(const Coroutine &other) = delete;

  Coroutine(std::coroutine_handle<promise_type> handle) : handle_(handle) {}

  Coroutine &operator=(const Coroutine &&other) noexcept {
    if (handle_) {
      handle_.destroy();
    }
    handle_ = std::move(other.handle_);
    return *this;
  }

  ~Coroutine() {
    if (handle_) {
      handle_.destroy();
    }
  }

  void step() { handle_.resume(); }

private:
  std::coroutine_handle<promise_type> handle_;
};
