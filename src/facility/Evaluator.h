#pragma once

#include <unordered_set>

#include "Types.h"

namespace facility {

inline double get_score(const Problem& problem, const Solution& solution) {
  double score = 0;
  std::unordered_set<size_t> opened_facilities;

  for (size_t i = 0; i < problem.customers.size(); ++i) {
    const size_t assigned_facility = solution.facility[i];

    score += distance(problem.facilities[assigned_facility].position,
                      problem.customers[i].position);

    opened_facilities.insert(assigned_facility);
  }

  for (const size_t facility : opened_facilities) {
    score += problem.facilities[facility].cost;
  }

  return score;
}

inline EvaluationResult evaluate(const Problem& problem,
                                 const Solution& solution) {
  if (solution.facility.size() != problem.customers.size()) {
    return EvaluationResult::invalid();
  }

  std::vector<double> total_demands(problem.facilities.size(), 0);

  for (size_t i = 0; i < problem.customers.size(); ++i) {
    const size_t assigned_facility = solution.facility[i];

    if (assigned_facility >= problem.facilities.size()) {
      return EvaluationResult::invalid();
    }

    total_demands[assigned_facility] += problem.customers[i].demand;
  }

  for (size_t i = 0; i < problem.facilities.size(); ++i) {
    if (total_demands[i] > problem.facilities[i].capacity) {
      return EvaluationResult::invalid();
    }
  }

  return EvaluationResult{
      .score = get_score(problem, solution),
      .is_valid = true,
  };
}

}  // namespace facility
