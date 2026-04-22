#pragma once

#include <vector>

#include "utils/Accumulators.h"
#include "vrp/Types.h"

namespace vrp {

class Greedy {
  const Problem& problem;

 public:
  explicit Greedy(const Problem& problem) : problem(problem) {}

  Solution solve() {
    std::vector<bool> served_customers(problem.customers.size(), false);
    std::vector<std::vector<size_t>> result(problem.vehicles_count);

    for (size_t i = 0; i < problem.vehicles_count; ++i) {
      size_t current_demand = 0;
      auto current_position = Problem::origin();

      // find the closest customer that fits
      while (true) {
        ArgMinimum<double, std::less<>> closest;

        for (size_t j = 0; j < problem.customers.size(); ++j) {
          // customer is already served
          if (served_customers[j]) {
            continue;
          }

          // customer doesn't fit into demand
          if (current_demand + problem.customers[j].demand >
              problem.vehicle_capacity) {
            continue;
          }

          closest.record(j, geom::distance(current_position,
                                           problem.customers[j].position));
        }

        if (!closest.argmin()) {
          break;
        }

        result[i].push_back(*closest.argmin());

        current_demand += problem.customers[*closest.argmin()].demand;
        current_position = problem.customers[*closest.argmin()].position;

        served_customers[*closest.argmin()] = true;
      }
    }

    return Solution{std::move(result)};
  }
};

}  // namespace vrp
