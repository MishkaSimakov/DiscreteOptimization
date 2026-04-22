#pragma once

#include <random>
#include <unordered_set>
#include <cassert>

#include "facility/Types.h"
#include "utils/Accumulators.h"

namespace facility {

// Process customers in order. For each customer assign cost to each available
// facility:
// 1. if facility is already assigned to different customer, then cost is
// distance
// 2. if facility is unassigned, then cost is the cost of opening + distance
class Greedy {
  const Problem& problem;

 public:
  explicit Greedy(const Problem& problem) : problem(problem) {}

  Solution solve() {
    std::vector<size_t> result(problem.customers.size(), 0);
    std::vector<double> current_demand(problem.facilities.size(), 0);

    for (size_t i = 0; i < problem.customers.size(); ++i) {
      ArgMinimum<double, std::less<>> min_cost;

      for (size_t j = 0; j < problem.facilities.size(); ++j) {
        if (current_demand[j] + problem.customers[i].demand >
            problem.facilities[j].capacity) {
          continue;
        }

        double cost = distance(problem.customers[i].position,
                               problem.facilities[j].position);

        if (current_demand[j] == 0) {
          cost += problem.facilities[j].cost;
        }

        min_cost.record(j, cost);
      }

      assert(min_cost.argmin().has_value());

      result[i] = *min_cost.argmin();
      current_demand[result[i]] += problem.customers[i].demand;
    }

    return Solution{std::move(result)};
  }
};

}  // namespace facility
