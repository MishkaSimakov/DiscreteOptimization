#pragma once

#include <concepts>
#include <optional>

namespace annealing {

template <typename T>
concept CoolingProcess = requires(T cooling) {
  { cooling.get_temperature() } -> std::same_as<double>;
  { cooling.advance() } -> std::same_as<std::optional<size_t>>;
};

}  // namespace annealing
