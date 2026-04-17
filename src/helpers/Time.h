#pragma once

#include <chrono>

namespace timing {

template <typename F>
std::chrono::nanoseconds timeit(F&& function) {
  auto t1 = std::chrono::high_resolution_clock::now();
  function();
  auto t2 = std::chrono::high_resolution_clock::now();

  return duration_cast<std::chrono::nanoseconds>(t2 - t1);
}

class Deadline {
  using Clock = std::chrono::steady_clock;

  Clock::time_point deadline_;

 public:
  explicit Deadline(Clock::time_point deadline) : deadline_(deadline) {}

  static Deadline after(Clock::duration duration) {
    return Deadline(Clock::now() + duration);
  }

  Clock::duration remaining_time() const {
    if (Clock::now() >= deadline_) {
      return std::chrono::nanoseconds{0};
    }

    return deadline_ - Clock::now();
  }

  bool is_over() const { return Clock::now() >= deadline_; }
};

}  // namespace timing
