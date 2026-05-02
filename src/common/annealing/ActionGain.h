#pragma once

namespace annealing {

// SimulatedAnnealing solves minimization problem.
// gain.score = old_score - new_score,
// gain.infeasibility = old_infeasibility - new_infeasibility,
// where infeasibility is the measure of constraints violation, it must be 0
// when none are violated.
struct ActionGain {
  double score;
  double infeasibility;
};

}  // namespace annealing
