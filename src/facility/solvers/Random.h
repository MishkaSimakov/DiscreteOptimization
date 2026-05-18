#pragma once

#include "Neighborhood.h"
#include "facility/Types.h"
#include "helpers/Random.h"

namespace facility {

class Random {
  constexpr static size_t neighborhood_size = 10;

  const Problem& problem;

  // customer-facility neighborhood
  const std::vector<std::vector<size_t>> neighbors;

  std::default_random_engine random_;

 public:
  explicit Random(const Problem& problem)
      : problem(problem),
        neighbors(get_customer_facility_neighborhood(problem, 10)) {}

  Solution solve() {
    const auto [n, d] = problem.shape();

    std::vector<size_t> result(d);

    for (size_t i = 0; i < d; ++i) {
      result[i] = neighbors[i][rnd::index(neighbors[i].size(), random_)];
    }

    return Solution{std::move(result)};
  }
};

}  // namespace facility
