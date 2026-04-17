#pragma once

#include <algorithm>
#include <vector>

#include "facility/Types.h"

namespace facility {

// For each customers returns up to max_count closest customers
inline std::vector<std::vector<size_t>> get_neighbors(const Problem& problem,
                                                      size_t max_count) {
  const auto [n, d] = problem.shape();
  std::vector<std::vector<size_t>> result(d);

  std::vector<size_t> heap;

  for (size_t i = 0; i < d; ++i) {
    auto proj = [i, &problem](size_t j) {
      return distance(problem.customers[i].position,
                      problem.customers[j].position);
    };

    for (size_t j = 0; j < d; ++j) {
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

}  // namespace facility
