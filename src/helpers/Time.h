#pragma once

#include <chrono>

namespace timing {

template <typename F>
std::chrono::milliseconds timeit(F&& function) {
  auto t1 = std::chrono::high_resolution_clock::now();
  function();
  auto t2 = std::chrono::high_resolution_clock::now();

  return duration_cast<std::chrono::milliseconds>(t2 - t1);
}

}  // namespace timing
