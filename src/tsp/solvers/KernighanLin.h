#pragma once

#include <algorithm>
#include <cassert>
#include <numeric>
#include <utility>

#include "helpers/Random.h"
#include "helpers/Time.h"
#include "tsp/Evaluator.h"
#include "tsp/Types.h"

namespace tsp {

class KernighanLin {
  std::default_random_engine random_;

  const Problem& problem;
  const timing::Deadline deadline;

  std::vector<std::vector<size_t>> candidates;

  // Chooses candidates list for the Kernighan Lin algorithm.
  // Takes closest max_count points.
  static auto get_candidates_by_distance(const Problem& problem,
                                         const size_t max_count) {
    const size_t n = problem.points.size();
    std::vector<std::vector<size_t>> result(n);

    std::vector<size_t> buffer(n);
    std::iota(buffer.begin(), buffer.end(), 0);

    for (size_t i = 0; i < n; ++i) {
      std::ranges::sort(buffer, {}, [&](size_t j) {
        return distance(problem.points[i], problem.points[j]);
      });

      for (size_t j = 0; j < max_count && j + 1 < n; ++j) {
        result[i].push_back(buffer[j + 1]);
      }
    }

    return result;
  }

  bool iteration(DoublyLinkedSolution& solution) {
    assert(evaluate(Solution{solution.next}).is_valid);

    const size_t n = problem.points.size();

    double best_improvement = 0;
    size_t best_improvement_k = 0;

    std::vector<size_t> nodes_chain;

    std::vector<bool> visited(n, false);

    nodes_chain.push_back(0);                 // t1
    nodes_chain.push_back(solution.next[0]);  // t2

    visited[0] = true;
    visited[solution.next[0]] = true;

    while (true) {
      const size_t t = nodes_chain.back();

      // choose y
      for (size_t i = 0; i < n; ++i) {
        if (i == t || visited[i] ||) }
    }

    return false;
  }

 public:
  KernighanLin(const Problem& problem, timing::Deadline deadline)
      : problem(problem), deadline(deadline) {}

  void compute_candidates() {
    if (candidates.empty()) {
      candidates = get_candidates_by_distance(problem, 5);
    }
  }

  Solution solve(const Solution& initial_solution) {
    auto current_solution = initial_solution;

    const size_t n = problem.points.size();

    double gain = 0;
    double cost = get_score(problem, current_solution);

    do {
      for (size_t t1 = 0; t1 < n; ++t1) {
        for (size_t direction = 0; direction < 2; ++direction) {
          const size_t t2 = direction == 0 ? current_solution.next[t1]
                                           : current_solution.prev[t1];

          double G = distance(problem.points[t1], problem.points[t2]);


        }
      }
    } while (!deadline.is_over() && gain > 0);

    return current_solution;
  }
};

}  // namespace tsp
