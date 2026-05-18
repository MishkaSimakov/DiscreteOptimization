#pragma once
#include "ProblemState.h"
#include "SolutionState.h"

#include "vrp/Evaluator.h"

namespace vrp {

inline double get_score(const ProblemState& problem,
                        const SolutionState& solution) {
  return get_score(problem.problem, solution.get_solution());
}

inline double get_infeasibility(const ProblemState& problem,
                                const SolutionState& solution) {
  return get_infeasibility(problem.problem, solution.get_solution());
}

}  // namespace vrp
