#pragma once

#include <algorithm>
#include <numeric>
#include <print>
#include <random>

#include "coloring/Types.h"
#include "helpers/Time.h"

// This class tries to find the largest clique
namespace coloring {
class LargestClique {
  timing::Deadline deadline_;

  std::vector<size_t> get_for_order(const Graph& problem,
                                    const std::vector<size_t>& order) {
    size_t n = problem.adjacent.size();

    std::vector<size_t> largest;
    std::vector<bool> clique(n, false);

    for (size_t i = 0; i < n; ++i) {
      size_t clique_size = 1;
      clique[order[i]] = true;

      for (size_t j = i + 1; j < n; ++j) {
        size_t clique_connections = 0;

        for (size_t adj : problem.adjacent[order[j]]) {
          if (clique[adj]) {
            ++clique_connections;
          }
        }

        if (clique_connections == clique_size) {
          ++clique_size;
          clique[order[j]] = true;
        }
      }

      if (clique_size > largest.size()) {
        largest.clear();

        for (size_t j = 0; j < n; ++j) {
          if (clique[j]) {
            largest.push_back(j);
          }
        }
      }

      std::fill_n(clique.begin(), n, false);
    }

    return largest;
  }

 public:
  LargestClique(timing::Deadline deadline) : deadline_(deadline) {}

  std::vector<size_t> get(const Graph& problem) {
    size_t n = problem.adjacent.size();

    std::default_random_engine engine_;

    std::vector<size_t> order(n);
    std::iota(order.begin(), order.end(), 0);

    std::vector<size_t> largest;

    while (true) {
      auto clique = get_for_order(problem, order);

      if (clique.size() > largest.size()) {
        largest = std::move(clique);
      }

      if (deadline_.is_over()) {
        break;
      }

      std::ranges::shuffle(order, engine_);
    }

    return largest;
  }
};

}  // namespace coloring
