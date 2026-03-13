#pragma once
#include <print>

#include "coloring/Types.h"

namespace coloring {
class ConnectedComponents {
  std::vector<size_t> components_;

  void dfs(size_t id, size_t component, const Graph& problem) {
    components_[id] = component;

    for (size_t adj : problem.adjacent[id]) {
      if (components_[adj] == problem.adjacent.size()) {
        dfs(adj, component, problem);
      }
    }
  }

 public:
  ConnectedComponents() = default;

  void apply(const Graph& problem) {
    size_t n = problem.adjacent.size();

    components_ = std::vector<size_t>(n, n);
    size_t component_count = 0;

    for (size_t i = 0; i < n; ++i) {
      if (components_[i] == n) {
        dfs(i, component_count, problem);
        ++component_count;
      }
    }

    std::println("  components: {}", component_count);
  }
};
}  // namespace coloring
