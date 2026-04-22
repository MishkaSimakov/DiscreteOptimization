#pragma once

#include <algorithm>

#include "tsp/Types.h"

/*
 * In big TSP problem it is infeasible to search through all node neighbors.
 * To reduce neighbors count only limited candidates set is considered for each
 * node.
 * There are multiple approaches to choosing candidates:
 * 1. The simplest one is to choose the closest nodes
 * 2. In the following paper a few approaches are described. They are harder to
 * implement but better than the first one.
 * http://webhotel4.ruc.dk/~keld/research/LKH/LKH-2.0/DOC/LKH_REPORT.pdf
 */

namespace tsp {

inline std::vector<std::vector<size_t>> get_candidates_by_distance(
    const Problem& problem, const size_t max_count) {
  const size_t n = problem.points.size();
  std::vector<std::vector<size_t>> result(n);

  std::vector<size_t> heap;

  for (size_t i = 0; i < n; ++i) {
    auto proj = [i, &problem](size_t j) {
      return distance(problem.points[i], problem.points[j]);
    };

    for (size_t j = 0; j < n; ++j) {
      if (j == i) {
        continue;
      }

      heap.push_back(j);
      std::ranges::push_heap(heap, {}, proj);

      if (heap.size() > max_count) {
        std::ranges::pop_heap(heap, {}, proj);
        heap.pop_back();
      }
    }

    result[i] = heap;
    heap.clear();
  }

  return result;
}

}  // namespace tsp
