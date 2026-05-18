#pragma once
#include "ProblemState.h"
#include "SolutionState.h"

#include "facility/Evaluator.h"

namespace facility {

inline double get_score(const ProblemState& problem,
                        const SolutionState& solution) {
  return get_score(problem.problem, solution.solution);
}

inline double get_infeasibility(const ProblemState& problem,
                                const SolutionState& solution) {
  return get_infeasibility(problem.problem, solution.solution);
}

}  // namespace facility
