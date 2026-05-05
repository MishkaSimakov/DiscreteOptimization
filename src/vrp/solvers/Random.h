#pragma once

#include <vector>

#include "helpers/Random.h"
#include "utils/Accumulators.h"
#include "vrp/Types.h"

namespace vrp {

// Generates random solution. It may not be feasible.
class Random {
  const Problem& problem;
  std::default_random_engine random_;

 public:
  explicit Random(const Problem& problem) : problem(problem) {}

  Solution solve() {
    std::vector<std::vector<size_t>> result(problem.vehicles_count);

    for (size_t i = 0; i < problem.customers.size(); ++i) {
      const size_t vehicle = rnd::index(problem.vehicles_count, random_);

      result[vehicle].push_back(i);
    }

    return Solution{std::move(result)};
  }
};

}  // namespace vrp
