#pragma once
#include "ProblemState.h"
#include "tsp/solvers/TourStorage.h"

namespace tsp {

struct SolutionState {
  TourStorage<> tour;

  SolutionState(const ProblemState& /* state */,
                const Solution& initial_solution)
      : tour(initial_solution) {}

  Solution get_solution() const { return tour.to_solution(); }
};

// SimulatedAnnealing in tsp operates in feasible solutions space, so
// infeasibility is always 0.
inline double get_infeasibility(const Problem& problem,
                                const Solution& solution) {
  return 0;
}

}  // namespace tsp
