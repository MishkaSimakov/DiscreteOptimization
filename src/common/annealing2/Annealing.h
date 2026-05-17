#pragma once

#include <chrono>
#include <concepts>
#include <random>
#include <vector>

#include "Moves.h"
#include "Random.h"
#include "detail/ScoredSolution.h"

namespace annealing {

template <typename T, typename Problem, typename Solution>
concept Scorer = requires(T scorer, Problem p, Solution s) {
  { scorer(p, s) } -> std::same_as<double>;
};

struct MoveStatistics {
  size_t proposed_transitions{0};
  size_t accepted_transitions{0};

  double get_acceptance_rate() const {
    return static_cast<double>(accepted_transitions) /
           static_cast<double>(proposed_transitions);
  }
};

inline double get_spent_ratio(std::chrono::nanoseconds spent,
                              std::chrono::nanoseconds total) {
  return std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(
             spent) /
         total;
}

// Applies Simulated Annealing to minimize score.
// Starts from the initial @solution, applies moves from @moves randomly.
// Returned solution may not be feasible, but it is guaranteed to be no worse
// than the initial @solution.
template <typename Problem, typename Solution, typename Scorer,
          typename Cooling>
Solution solve(Problem problem, Solution solution, Scorer scorer,
               Moves<Problem, Solution, Scorer> moves, Cooling cooling,
               std::chrono::nanoseconds duration) {
  constexpr size_t temperature_change_period = 100;

  Random random;

  detail::ScoredSolution<Solution> current{
      .solution = solution,
      .score = scorer(problem, solution),
  };

  detail::ScoredSolution<Solution> best = current;

  using Clock = std::chrono::steady_clock;
  const auto start = Clock::now();

  size_t iteration = 0;
  double current_temperature = cooling.get_temperature(0);

  while (true) {
    if ((iteration + 1) % temperature_change_period == 0) {
      const double ratio = get_spent_ratio(Clock::now() - start, duration);

      if (ratio > 1.) {
        break;
      }

      current_temperature = cooling.get_temperature(ratio);
    }

    const bool applied =
        moves.try_apply(problem, current, current_temperature, scorer, random);

    if (applied && current.score < best.score - 1e-5) {
      best = current;
    }

    ++iteration;
  }

  return best.solution;
}

}  // namespace annealing
