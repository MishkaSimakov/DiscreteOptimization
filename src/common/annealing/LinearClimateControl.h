#pragma once

namespace annealing {

class LinearClimateControl {
  double temperature_;

 public:
  LinearClimateControl() = default;

  double get_temperature() const { return temperature_; }
};

}  // namespace annealing
