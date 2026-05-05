#pragma once

#include <vector>

#include "common/Neighbors.h"
#include "vrp/Types.h"

namespace vrp {

struct ProblemState {
  constexpr static size_t candidates_count = 5;

  const Problem& problem;
  std::vector<std::vector<size_t>> candidates;

  static auto get_candidates(const Problem& problem) {
    auto points =
        problem.customers | std::views::transform([](Customer customer) {
          return customer.position;
        });

    return get_neighbors_by_distance(points, candidates_count);
  }

  explicit ProblemState(const Problem& problem)
      : problem(problem), candidates(get_candidates(problem)) {}

  const Problem& get_problem() const { return problem; }

  double get_distance(size_t i, size_t j) const {
    const auto a = i >= problem.customers.size()
                       ? problem.origin
                       : problem.customers[i].position;
    const auto b = j >= problem.customers.size()
                       ? problem.origin
                       : problem.customers[j].position;

    return geom::distance(a, b);
  }
};

}  // namespace vrp
