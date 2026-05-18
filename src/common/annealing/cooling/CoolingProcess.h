#pragma once

#include <concepts>

namespace annealing {

template <typename T>
concept CoolingProcess = requires(T cooling, double time) {
  { cooling.get_temperature(time) } -> std::same_as<double>;
};

}  // namespace annealing
