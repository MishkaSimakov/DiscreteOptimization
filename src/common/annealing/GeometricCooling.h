#pragma once

#include <optional>
#include <vector>

#include "CoolingProcess.h"

namespace annealing {

class GeometricCooling {
  size_t index_;
  std::vector<double> temperatures_;

 public:
  GeometricCooling(double start_temperature, double alpha, size_t steps_count)
      : index_(0), temperatures_(steps_count) {
    if (steps_count == 0) {
      throw std::invalid_argument("steps_count must be strictly positive.");
    }

    double current_temperature = start_temperature;

    for (size_t i = 0; i < steps_count; ++i) {
      temperatures_[i] = current_temperature;
      current_temperature *= alpha;
    }
  }

  double get_temperature() const { return temperatures_[index_]; }

  // Returns std::nullopt when cannot advance any further,
  // returns remaining steps count otherwise.
  std::optional<size_t> advance() {
    if (index_ + 1 == temperatures_.size()) {
      return std::nullopt;
    }

    ++index_;
    return temperatures_.size() - index_ - 1;
  }
};

static_assert(CoolingProcess<GeometricCooling>);

}  // namespace annealing
