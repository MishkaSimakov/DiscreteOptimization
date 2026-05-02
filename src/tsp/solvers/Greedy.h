#pragma once

#include <algorithm>
#include <utility>

#include "Candidates.h"
#include "tsp/Types.h"
#include "utils/Accumulators.h"

namespace tsp {

class Greedy {
  const Problem& problem;
  const std::vector<std::vector<size_t>> candidates;

  std::default_random_engine random_;

 public:
  explicit Greedy(const Problem& problem, size_t candidates_count = 5)
      : problem(problem),
        candidates(get_candidates_by_distance(problem, candidates_count)) {}

  Solution solve() {
    const size_t n = problem.points.size();

    std::vector<size_t> next(n, -1);
    const size_t start = rnd::index(n, random_);
    size_t current = start;

    for (size_t i = 0; i < n - 1; ++i) {
      // choose closest to the current
      ArgMinimum<double> closest;

      // first go through candidates
      for (const size_t j : candidates[current]) {
        if (j == current || next[j] != -1) {
          continue;
        }

        closest.record(j, distance(problem.points[current], problem.points[j]));
      }

      if (closest.has_value()) {
        next[current] = closest->index;
        current = next[current];

        continue;
      }

      // if failed to find node among candidates, perform full search
      for (size_t j = 0; j < n; ++j) {
        if (j == current || next[j] != -1) {
          continue;
        }

        closest.record(j, distance(problem.points[current], problem.points[j]));
      }

      next[current] = closest->index;
      current = next[current];
    }

    next[current] = start;

    return Solution{std::move(next)};
  }
};

}  // namespace tsp
