#pragma once

#include "ScoredSolution.h"
#include "common/annealing2/Random.h"

namespace annealing::detail {

template <typename Problem, typename Solution, typename Scorer>
class IBoxedMove {
 public:
  // Samples random move, applies it with Simulated Annealing
  // probability model. Returns whether the move was applied.
  virtual bool try_apply(const Problem& problem,
                         ScoredSolution<Solution>& solution,
                         const Scorer& scorer, double temperature,
                         Random& random) = 0;

  // Samples random move and returns its gain.
  // May change solution inside this method, but after the call solution would
  // always be the same as before the call.
  virtual double get_gain(const Problem& problem,
                          ScoredSolution<Solution>& solution,
                          const Scorer& scorer, Random& random) = 0;

  virtual ~IBoxedMove() = default;
};

}  // namespace annealing::detail
