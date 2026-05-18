#pragma once

#include <string_view>
#include <unordered_map>

#include "Acceptance.h"
#include "ScoredSolution.h"

namespace annealing {

template <typename ProblemState, typename SolutionState>
struct LoggingContext {
  const ProblemState& problem;

  const ScoredSolution<SolutionState>& current;
  const ScoredSolution<SolutionState>& best;

  double temperature;
  double infeasibility_penalty;
  size_t changes_count;

  const std::unordered_map<std::string, ActionAcceptance>& acceptances;
};

}  // namespace annealing
