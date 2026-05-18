#pragma once

#include <print>

#include "LoggingContext.h"

namespace annealing {

template <typename Problem, typename Solution>
void log_best(const LoggingContext<Problem, Solution>& context) {
  if (context.best.infeasibility > 0) {
    std::println("  [.] new best: {:.5f}\t(infeasibility = {})",
                 context.best.score, context.best.infeasibility);
  } else {
    std::println("  [!] new best: {:.5f}\t(feasible)", context.best.score);
  }
}

template <typename Solution, typename Problem>
void log_state(const LoggingContext<Solution, Problem>& context) {
  std::println("hello world!");
}

}  // namespace annealing
