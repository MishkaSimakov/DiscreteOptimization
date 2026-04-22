#pragma once
#include <random>

#include "helpers/Time.h"
#include "vrp/Types.h"

namespace vrp {

class LocalSearch {
  const Problem& problem;
  timing::Deadline deadline;

 public:
  explicit LocalSearch(const Problem& problem, timing::Deadline deadline)
      : problem(problem), deadline(deadline) {}

  Solution solve() {
    std::default_random_engine random;

    // construct random initial assignment
    std::vector<size_t> assignment(problem.customers.size());
    for (size_t i = 0; i < problem.customers.size(); ++i) {
      assignment[i] = random() % problem.vehicles_count;
    }

    std::vector<size_t> demands(problem.vehicles_count, 0);
    for (size_t i = 0; i < problem.customers.size(); ++i) {
      demands[assignment[i]] += problem.customers[i].demand;
    }


    while (true) {

    }
  }
};

}  // namespace vrp
