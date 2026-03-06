#pragma once

#include "Types.h"

namespace knapsack {

EvaluationResult evaluate(const Problem& problem, const Solution& solution) {
  size_t total_cost = 0;
  size_t total_weight = 0;

  for (size_t item : solution.chosen_items) {
    total_cost += problem.items[item].cost;
    total_weight += problem.items[item].weight;
  }

  return EvaluationResult{
      .score = total_cost,
      .is_valid = total_weight <= problem.max_weight,
  };
}

}  // namespace knapsack
