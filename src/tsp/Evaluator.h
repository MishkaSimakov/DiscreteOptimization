#pragma once

#include <unordered_set>

#include "Types.h"

namespace tsp {

inline double get_score(const Problem& problem, const Solution& solution) {
  const size_t n = problem.points.size();

  double score = 0;

  for (size_t i = 0; i < n; ++i) {
    score += distance(problem.points[i], problem.points[solution.next[i]]);
  }

  return score;
}

inline EvaluationResult evaluate(const Problem& problem,
                                 const Solution& solution) {
  const size_t n = problem.points.size();

  std::unordered_set<size_t> visited_points;
  size_t current = 0;

  for (size_t i = 0; i < n; ++i) {
    auto [_, inserted] = visited_points.emplace(current);

    if (!inserted) {
      return EvaluationResult::invalid();
    }

    current = solution.next[current];
  }

  if (current != 0) {
    return EvaluationResult::invalid();
  }

  return EvaluationResult{
      .score = get_score(problem, solution),
      .is_valid = true,
  };
}

}  // namespace tsp
