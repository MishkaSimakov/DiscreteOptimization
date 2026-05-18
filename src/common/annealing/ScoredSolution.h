#pragma once

namespace annealing {

// Score function calculation may be expensive. Therefore, solution is stored
// with its score where it's possible.
template <typename Solution>
struct ScoredSolution {
  Solution solution;
  double score;
  double infeasibility;

  auto operator<=>(const ScoredSolution& other) const {
    return std::pair{infeasibility, score} <=>
           std::pair{other.infeasibility, other.score};
  }
};

}  // namespace annealing
