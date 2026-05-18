#pragma once

#include <cmath>
#include <stdexcept>

#include "CoolingProcess.h"

namespace annealing {

class GeometricCooling {
  double start_;
  double end_;

 public:
  GeometricCooling(double start_temperature, double end_temperature)
      : start_(start_temperature), end_(end_temperature) {
    if (end_temperature > start_temperature) {
      throw std::invalid_argument(
          "Start temperature must be higher than end temperature.");
    }
  }

  double get_temperature(double t) const {
    return start_ * std::pow(end_ / start_, t);
  }
};

static_assert(CoolingProcess<GeometricCooling>);

}  // namespace annealing
