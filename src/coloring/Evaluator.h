#pragma once

#include "Types.h"

namespace coloring {

inline EvaluationResult evaluate(const Graph& problem,
                                 const Solution& solution) {
  size_t colors_count = 0;

  for (size_t i = 0; i < problem.adjacent.size(); ++i) {
    colors_count = std::max(colors_count, solution.colors[i] + 1);

    for (const size_t neighbor : problem.adjacent[i]) {
      if (solution.colors[i] == solution.colors[neighbor]) {
        return EvaluationResult{.score = 0, .is_valid = false};
      }
    }
  }

  return EvaluationResult{.score = colors_count, .is_valid = true};
}

}  // namespace coloring
