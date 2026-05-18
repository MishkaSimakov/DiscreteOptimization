#pragma once

#include <string_view>
#include <unordered_map>

#include "Acceptance.h"
#include "ScoredSolution.h"

namespace annealing {

template <typename Problem, typename Solution>
struct LoggingContext {
  const Problem& problem;

  const ScoredSolution<Solution>& current;
  const ScoredSolution<Solution>& best;

  double temperature;
  double infeasibility_penalty;

  size_t iterations_count;
  size_t changes_count;

  const std::unordered_map<std::string, ActionAcceptance>& acceptances;
};

}  // namespace annealing
