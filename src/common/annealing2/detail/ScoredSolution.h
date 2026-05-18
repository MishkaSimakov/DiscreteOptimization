#pragma once

namespace annealing::detail {

template <typename Solution>
struct ScoredSolution {
  Solution solution;
  double score;
};

}  // namespace annealing::detail
