#pragma once
#include "ProblemState.h"
#include "tsp/solvers/TourStorage.h"

namespace tsp {

struct SolutionState {
  TourStorage<> tour;

  explicit SolutionState(const Solution& solution) : tour(solution) {}

  Solution get_solution() const { return tour.to_solution(); }
};

// SimulatedAnnealing in tsp operates in feasible solutions space, so
// infeasibility is always 0.
inline double get_infeasibility(const ProblemState& problem,
                                const SolutionState& solution) {
  return 0;
}

inline double get_score(const ProblemState& problem,
                        const SolutionState& solution) {
  return get_score(problem.problem, solution.tour);
}

}  // namespace tsp
