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
    std::vector<geom::Point<double>> points(problem.customers.size() +
                                            problem.vehicles_count);

    for (size_t i = 0; i < problem.customers.size(); ++i) {
      points[i] = problem.customers[i].position;
    }

    for (size_t i = 0; i < problem.vehicles_count; ++i) {
      points[problem.customers.size() + i] = problem.origin;
    }

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
