#pragma once

#include <print>

#include "LoggingContext.h"

namespace annealing {

template <typename ProblemState, typename SolutionState>
void log_best(const LoggingContext<ProblemState, SolutionState>& context) {
  if (context.best.infeasibility > 0) {
    std::println("  [.] new best: {:.5f}\t(infeasibility = {})",
                 context.best.score, context.best.infeasibility);
  } else {
    std::println("  [!] new best: {:.5f}\t(feasible)", context.best.score);
  }
}

template <typename SolutionState, typename ProblemState>
void log_state(const LoggingContext<SolutionState, ProblemState>& context) {
  std::println("hello world!");
}

}  // namespace annealing
