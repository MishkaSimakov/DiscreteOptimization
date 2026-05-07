#pragma once

namespace annealing {

class InfeasibilityController {
  const double penalty;

 public:
  explicit InfeasibilityController(const double infeasibility_penalty)
      : penalty(infeasibility_penalty) {}

  double get_penalty() const { return penalty; }

  void update(const double infeasibility, const double temperature) {}
};

}  // namespace annealing
