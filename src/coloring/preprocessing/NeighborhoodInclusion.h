#pragma once
#include <numeric>
#include <print>

#include "coloring/Types.h"

namespace coloring {

class NeighborhoodInclusion {
  // buffer must be filled with n zeros
  void bfs(size_t root, std::vector<size_t>& mapping, const Graph& problem,
           std::vector<size_t>& buffer) {
    std::vector<size_t> second_stage;

    buffer[root] = 1;
    for (size_t adj : problem.adjacent[root]) {
      buffer[adj] = 1;
    }

    for (size_t adj : problem.adjacent[root]) {
      for (size_t adj_adj : problem.adjacent[adj]) {
        if (buffer[adj_adj] == 0) {
          buffer[adj_adj] = 2;
          second_stage.push_back(adj_adj);
        }
      }
    }

    for (size_t adj_adj : second_stage) {
      bool is_valid = true;
      for (size_t i : problem.adjacent[adj_adj]) {
        if (buffer[i] != 1) {
          is_valid = false;
          break;
        }
      }

      if (is_valid) {
        mapping[adj_adj] = root;
        std::println("{} -> {}", adj_adj, root);
      }
    }

    // restore buffer to its initial state
    buffer[root] = 0;
    for (size_t adj_adj : second_stage) {
      buffer[adj_adj] = 0;
    }
    for (size_t adj : problem.adjacent[root]) {
      buffer[adj] = 0;
    }
  }

 public:
  NeighborhoodInclusion() = default;

  void apply(const Graph& problem) {
    size_t n = problem.adjacent.size();

    // mapping[i] = j means that i can be colored in the same color as j
    std::vector<size_t> mapping(n);
    std::iota(mapping.begin(), mapping.end(), 0);

    std::vector<size_t> buffer(n, 0);

    for (size_t i = 0; i < n; ++i) {
      bfs(i, mapping, problem, buffer);
    }
  }
};

}  // namespace coloring
