#pragma once

#include <ranges>
#include <vector>

#include "common/Neighbors.h"
#include "facility/Types.h"

namespace facility {

struct ProblemState {
  constexpr static size_t candidates_count = 5;

  const Problem& problem;

  // customer-customer neighborhood
  std::vector<std::vector<size_t>> neighbors;

  explicit ProblemState(const Problem& problem)
      : problem(problem),
        neighbors(get_neighbors_by_distance(
            problem.customers |
                std::views::transform(
                    [](const Customer customer) { return customer.position; }),
            candidates_count)) {}

  const Problem& get_problem() const { return problem; }
};

}  // namespace facility
