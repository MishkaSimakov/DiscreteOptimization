#pragma once

namespace annealing {

// SimulatedAnnealing solves minimization problem.
// score_gain = old_score - new_score,
// infeasibility_gain = old_infeasibility - new_infeasibility,
// where infeasibility is the measure of constraints violation, it must be 0
// when none are violated.
struct ActionGain {
  double score_gain;
  double infeasibility_gain;
};

}  // namespace annealing
