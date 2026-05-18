#pragma once

namespace annealing {

// This solver state is shared with actions managers.
struct SolverStateDTO {
  // may be handy for tabu list implementation
  size_t changes_count;
};

// This is private solver state, it is used in implementation
template <typename Solution>
struct InternalStateDTO {
  Solution& solution;
  double temperature;
  double infeasibility_penalty;
  std::default_random_engine& random;
  size_t changes_count;
};

}  // namespace annealing
