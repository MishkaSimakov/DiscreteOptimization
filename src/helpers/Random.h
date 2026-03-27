#pragma once

#include <algorithm>
#include <cassert>
#include <random>

namespace rnd {

template <typename Gen>
  requires std::uniform_random_bit_generator<std::remove_reference_t<Gen>>
size_t index(size_t size, Gen&& generator) {
  assert(size != 0);

  return std::uniform_int_distribution<size_t>(0, size - 1)(generator);
}

}  // namespace rnd
