#pragma once

namespace coloring {

class Greedy {
 public:
  Greedy() = default;

  Solution solve(const Graph& problem) {
    size_t n = problem.adjacent.size();

    std::vector<size_t> colors(n, n);
    std::vector<bool> occupied(n + 1, false);

    for (size_t i = 0; i < n; ++i) {
      for (size_t adj : problem.adjacent[i]) {
        occupied[colors[adj]] = true;
      }

      for (size_t j = 0; j < n; ++j) {
        if (!occupied[j]) {
          colors[i] = j;
          break;
        }
      }

      for (size_t adj : problem.adjacent[i]) {
        occupied[colors[adj]] = false;
      }
    }

    return Solution{colors};
  }
};

}  // namespace coloring
