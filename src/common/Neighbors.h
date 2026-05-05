#pragma once

#include <algorithm>
#include <ranges>
#include <vector>

#include "helpers/Geometry.h"

template <typename R>
  requires std::ranges::random_access_range<R> && std::ranges::sized_range<R>
std::vector<std::vector<size_t>> get_neighbors_by_distance(
    R&& points, const size_t max_count) {
  const size_t n = std::ranges::size(points);
  std::vector<std::vector<size_t>> result(n);

  std::vector<size_t> heap;

  for (size_t i = 0; i < n; ++i) {
    auto proj = [i, &points](size_t j) {
      return geom::distance_sqr(points[i], points[j]);
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

    result[i] = std::move(heap);
    heap.clear();
  }

  return result;
}
