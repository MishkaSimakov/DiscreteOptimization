#pragma once

#include <unordered_set>

#include "Types.h"

namespace tsp {

inline double get_score(const Problem& problem, const Solution& solution) {
  double score = 0;

  for (size_t i = 0; i < solution.order.size(); ++i) {
    size_t next = (i + 1) % solution.order.size();

    score += distance(problem.points[solution.order[i]],
                      problem.points[solution.order[next]]);
  }

  return score;
}

inline EvaluationResult evaluate(const Problem& problem,
                                 const Solution& solution) {
  const size_t n = problem.points.size();

  std::unordered_set<size_t> visited_points;
  for (size_t i : solution.order) {
    visited_points.emplace(i);
  }

  // each point is visited exactly once
  if (visited_points.size() != n || solution.order.size() != n) {
    return EvaluationResult{
        .score = 0,
        .is_valid = false,
    };
  }

  return EvaluationResult{
      .score = get_score(problem, solution),
      .is_valid = true,
  };
}

}  // namespace tsp
