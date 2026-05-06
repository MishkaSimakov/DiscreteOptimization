#pragma once

namespace annealing {

class InfeasibilityController {
  static constexpr double base_infeasibility_penalty = 50;

  double penalty_;
  double integral_;

 public:
  InfeasibilityController()
      : penalty_(base_infeasibility_penalty), integral_(0) {}

  double get_penalty() const {
    return penalty_;
  }

  void update(const double infeasibility, const double temperature) {
    // if (infeasibility == 0) {
    //   integral_ = 0;
    // } else {
    //   integral_ += infeasibility;
    // }
    //
    // penalty_ = base_infeasibility_penalty * std::exp(0.001 * integral_);
    // penalty_ = std::min(1e6, penalty_);
    penalty_ = 100. / temperature;
  }
};

}  // namespace annealing
