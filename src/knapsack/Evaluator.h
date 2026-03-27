#pragma once

#include "Types.h"

namespace knapsack {

inline size_t get_weight(const Problem& problem, const Solution& solution) {
  size_t total_weight = 0;

  for (const size_t item : solution.chosen_items) {
    total_weight += problem.items[item].weight;
  }

  return total_weight;
}

inline size_t get_score(const Problem& problem, const Solution& solution) {
  size_t total_cost = 0;

  for (const size_t item : solution.chosen_items) {
    total_cost += problem.items[item].cost;
  }

  return total_cost;
}

inline EvaluationResult evaluate(const Problem& problem,
                                 const Solution& solution) {
  return EvaluationResult{
      .score = get_score(problem, solution),
      .is_valid = get_weight(problem, solution) <= problem.max_weight,
  };
}

}  // namespace knapsack
