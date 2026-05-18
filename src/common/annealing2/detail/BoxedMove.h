#pragma once

#include <utility>

#include "IBoxedMove.h"
#include "ScoredSolution.h"
#include "common/annealing2/Random.h"

namespace annealing::detail {

template <typename Problem, typename Solution, typename Scorer,
          typename MoveType>
class BoxedMove final : public IBoxedMove<Problem, Solution, Scorer> {
 public:
  bool try_apply(const Problem& problem, ScoredSolution<Solution>& solution,
                 const Scorer& scorer, double temperature,
                 Random& random) override {
    const auto move =
        MoveType::generate(problem, std::as_const(solution.solution), random);

    if (!move) {
      return false;
    }

    double gain;

    if constexpr (true) {
      // supports incremental score update
      // TODO:
      gain = 123;
    } else {
      MoveType::apply(problem, solution.solution, *move);

      const double new_score =
          scorer(problem, std::as_const(solution.solution));

      MoveType::revert(problem, solution.solution, *move);

      gain = solution.score - new_score;
    }

    // accept using simulated annealing algorithm
    std::uniform_real_distribution<double> prob(0, 1);

    if (gain > 0 || std::exp(gain / temperature) > prob(random)) {
      MoveType::apply(problem, solution.solution, *move);

      return true;
    }

    return false;
  }

  double get_gain(const Problem& problem, ScoredSolution<Solution>& solution,
                  const Scorer& scorer, Random& random) override {
    const auto move =
        MoveType::generate(problem, std::as_const(solution.solution), random);

    if (!move) {
      return 0;
    }

    if constexpr (true) {
      // supports incremental score update
      // TODO:
      return 123;
    } else {
      MoveType::apply(problem, solution.solution, *move);

      const double new_score =
          scorer(problem, std::as_const(solution.solution));

      MoveType::revert(problem, solution.solution, *move);

      return solution.score - new_score;
    }
  }
};

}  // namespace annealing::detail
