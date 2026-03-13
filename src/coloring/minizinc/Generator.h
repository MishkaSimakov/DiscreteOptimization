#pragma once

#include <iostream>
#include <ranges>

#include "coloring/Types.h"
#include "utils/String.h"

namespace coloring {

// Generates minizinc dzn files for graph coloring problem
class Generator {
 public:
  Generator() = default;

  void to_dzn(std::ostream& os, const Graph& problem) {
    size_t n = problem.adjacent.size();
    size_t m = 0;

    std::string edges_description;

    for (size_t i = 0; i < n; ++i) {
      for (size_t j : problem.adjacent[i]) {
        if (i < j) {
          edges_description += std::format("{}, {}|\n", i, j);
          ++m;
        }
      }
    }

    std::println(os, "NUM_NODES = {};", n);
    std::println(os, "NUM_EDGES = {};", m);
    std::println(os, "edges = [|\n{}|];", edges_description);

    // clique for symmetry breaking
    auto clique =
        LargestClique(timing::Deadline::after(std::chrono::seconds{1}))
            .get(problem);

    std::string clique_description = str::join(
        clique |
            std::views::transform([](size_t i) { return std::to_string(i); }),
        ", ");

    std::println(os, "NUM_CLIQUE_NODES = {};", clique.size());
    std::println(os, "clique = [{}];", clique_description);
  }
};

}  // namespace coloring
