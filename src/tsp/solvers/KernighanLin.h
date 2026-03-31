#pragma once

#include <algorithm>
#include <utility>

#include "helpers/Time.h"
#include "tsp/Types.h"

namespace tsp {

class KernighanLin {
  const Problem& problem;
  const timing::Deadline deadline;

  const std::vector<std::vector<size_t>> sorted_neighbors;

  static auto get_sorted_neighbors(const Problem& problem) {
    const size_t n = problem.points.size();
    std::vector<std::vector<size_t>> result(n);

    for (size_t i = 0; i < n; ++i) {
      result[i].reserve(n - 1);

      for (size_t j = 0; j < n; ++j) {
        if (j != i) {
          result[i].push_back(j);
        }
      }

      std::ranges::sort(result[i], {}, [&](size_t j) {
        return distance(problem.points[i], problem.points[j]);
      });
    }

    return result;
  }

  // returns the closest point to solution.order[index] that is not connected
  // to it in solution
  size_t get_closest_unconnected(const Solution& solution, size_t index) const {
    const size_t n = problem.points.size();
    const size_t node = solution.order[index];

    for (size_t i = 0; i < n - 1; ++i) {
      const size_t candidate = sorted_neighbors[node][i];

      if (candidate != solution.order[(index + 1) % n] &&
          candidate != solution.order[(index + n - 1) % n]) {
        return candidate;
      }
    }

    std::unreachable();
  }

  bool iteration(Solution& solution) {
    assert(evaluate(solution).is_valid);

    const size_t n = problem.points.size();

    double best_improvement = 0;
    size_t best_improvement_k = 0;

    std::vector<size_t> nodes_chain;

    for (size_t t1_index = 0; t1_index < n; ++t1_index) {
      const size_t t1 = solution.order[t1_index];

      const size_t t2_index = (t1_index + 1) % n;
      const size_t t2 = solution.order[t2_index];

      const size_t t3 = get_closest_unconnected(solution, (t1_index + 1));
    }

    return false;
  }

 public:
  KernighanLin(const Problem& problem, timing::Deadline deadline)
      : problem(problem),
        deadline(deadline),
        sorted_neighbors(get_sorted_neighbors(problem)) {}

  Solution solve(const Solution& initial_solution) {
    Solution current_solution = initial_solution;

    while (true) {
      if (deadline.is_over()) {
        break;
      }
    }

    return current_solution;
  }
};

}  // namespace tsp
