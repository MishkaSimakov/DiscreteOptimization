#pragma once

#include <unordered_set>

#include "Types.h"

namespace vrp {

inline double get_score(const Problem& problem, const Solution& solution) {
  double distance = 0;

  for (const auto& route : solution.routes) {
    distance +=
        geom::distance(Problem::origin(), problem.customers[route[0]].position);
    distance += geom::distance(Problem::origin(),
                               problem.customers[route.back()].position);

    for (size_t i = 0; i + 1 < route.size(); ++i) {
      distance += geom::distance(problem.customers[route[i]].position,
                                 problem.customers[route[i + 1]].position);
    }
  }

  return distance;
}

inline EvaluationResult evaluate(const Problem& problem,
                                 const Solution& solution) {
  if (solution.routes.size() != problem.vehicles_count) {
    return EvaluationResult::invalid();
  }

  std::unordered_set<size_t> served_customers;

  for (const auto& route : solution.routes) {
    size_t vehicle_demand = 0;

    for (const size_t customer : route) {
      if (customer >= problem.customers.size()) {
        return EvaluationResult::invalid();
      }

      auto [itr, inserted] = served_customers.insert(customer);

      if (!inserted) {
        // one customer is served twice
        return EvaluationResult::invalid();
      }

      vehicle_demand += problem.customers[customer].demand;
    }

    if (vehicle_demand > problem.vehicle_capacity) {
      return EvaluationResult::invalid();
    }
  }

  if (served_customers.size() != problem.customers.size()) {
    // someone was not served
    return EvaluationResult::invalid();
  }

  return EvaluationResult{
      .score = get_score(problem, solution),
      .is_valid = true,
  };
}

}  // namespace vrp
