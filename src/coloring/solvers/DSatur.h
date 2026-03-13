#pragma once

#include <cassert>

#include "coloring/Types.h"
#include "utils/Accumulators.h"

namespace coloring {

class DSatur {
 public:
  DSatur() = default;

  Solution solve(const Graph& problem) {
    size_t n = problem.adjacent.size();

    std::vector<size_t> colors(n, n);
    std::vector<bool> occupied(n + 1, false);

    // (# of distinct colors among neighbors, # of uncolored neighbors)
    std::vector<std::pair<size_t, size_t>> saturation(n);
    for (size_t i = 0; i < n; ++i) {
      saturation[i] = {0, problem.adjacent[i].size()};
    }

    for (size_t i = 0; i < n; ++i) {
      // choose next node based on saturation
      ArgMaximum<std::pair<size_t, size_t>, std::less<>> max_saturation;
      for (size_t j = 0; j < n; ++j) {
        if (colors[j] == n) {
          max_saturation.record(j, saturation[j]);
        }
      }

      assert(max_saturation.argmax().has_value());

      size_t node = *max_saturation.argmax();

      // color node using greedy coloring and update saturation
      for (size_t adj : problem.adjacent[node]) {
        occupied[colors[adj]] = true;
      }

      for (size_t j = 0; j < n; ++j) {
        if (!occupied[j]) {
          colors[node] = j;
          break;
        }
      }

      for (size_t adj : problem.adjacent[node]) {
        occupied[colors[adj]] = false;

        bool is_new = true;
        for (size_t adj_adj : problem.adjacent[adj]) {
          if (adj_adj != node && colors[adj_adj] == colors[node]) {
            is_new = false;
            break;
          }
        }

        if (is_new) {
          ++saturation[adj].first;
        }
        --saturation[adj].second;
      }
    }

    return Solution{colors};
  }
};

}  // namespace coloring
