#pragma once

#include <algorithm>
#include <utility>

#include "helpers/Time.h"
#include "tsp/Types.h"
#include "utils/Accumulators.h"

namespace tsp {

class Greedy {
  const Problem& problem;

  size_t get_closest_unvisited(size_t node,
                               const std::vector<size_t>& next) const {
    const size_t n = problem.points.size();

    ArgMinimum<double> closest;

    for (size_t i = 0; i < n; ++i) {
      if (i == node || next[i] != -1) {
        continue;
      }

      closest.record(i, distance(problem.points[i], problem.points[node]));
    }

    return *closest.argmin();
  }

 public:
  explicit Greedy(const Problem& problem) : problem(problem) {}

  Solution solve() const {
    const size_t n = problem.points.size();

    std::vector<size_t> result(n, -1);

    size_t current = 0;
    for (size_t i = 0; i < n - 1; ++i) {
      const size_t next = get_closest_unvisited(current, result);

      result[current] = next;
      current = next;
    }

    result[current] = 0;

    return Solution{result};
  }
};

}  // namespace tsp
