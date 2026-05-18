#pragma once

namespace annealing {

struct ActionAcceptance {
  size_t proposed_transitions{0};
  size_t accepted_transitions{0};

  double get_rate() const {
    return static_cast<double>(accepted_transitions) /
           static_cast<double>(proposed_transitions);
  }
};

}  // namespace annealing
