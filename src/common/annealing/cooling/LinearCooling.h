#pragma once

#include <stdexcept>

#include "CoolingProcess.h"

namespace annealing {

class LinearCooling {
  double start_;
  double end_;

 public:
  LinearCooling(double start_temperature, double end_temperature)
      : start_(start_temperature), end_(end_temperature) {
    if (end_temperature > start_temperature) {
      throw std::invalid_argument(
          "Start temperature must be higher than end temperature.");
    }
  }

  double get_temperature(double t) const { return start_ * (1 - t) + end_ * t; }
};

static_assert(CoolingProcess<LinearCooling>);

}  // namespace annealing
