#pragma once
#include "tsp/solvers/TourStorage.h"

namespace tsp {

struct ProblemState {
  constexpr static size_t candidates_count = 5;

  const Problem& problem;
  std::vector<std::vector<size_t>> candidates;

  explicit ProblemState(const Problem& problem)
      : problem(problem),
        candidates(get_candidates_by_distance(problem, candidates_count)) {}

  const Problem& get_problem() const { return problem; }
};

}  // namespace tsp
