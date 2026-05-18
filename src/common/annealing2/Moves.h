#pragma once

#include <memory>
#include <string_view>

#include "detail/BoxedMove.h"
#include "detail/IBoxedMove.h"

namespace annealing {

// A man is known by his deeds, whilst Simulated Annealing is known by its moves
template <typename Problem, typename Solution, typename Scorer>
class Moves {
  struct MoveTypeEntry {
    std::string_view name;
    std::unique_ptr<detail::IBoxedMove<Problem, Solution, Scorer>> move;
    double weight;
  };

  std::vector<MoveTypeEntry> moves_;
  std::discrete_distribution<size_t> distribution_;

  void update_distribution() {
    std::vector<double> weights(moves_.size());

    for (size_t i = 0; i < moves_.size(); ++i) {
      weights[i] = moves_[i].weight;
    }

    distribution_ =
        std::discrete_distribution<size_t>(weights.begin(), weights.end());
  }

 public:
  template <typename MoveType>
  void add(std::string_view name, double weight) {
    moves_.emplace_back(
        name,
        std::make_unique<
            detail::BoxedMove<Problem, Solution, Scorer, MoveType>>(),
        weight);

    update_distribution();
  }

  bool try_apply(const Problem& problem, Solution& solution,
                 const Scorer& scorer, double temperature, Random& random) {
    const size_t random_move = distribution_(random);

    return moves_[random_move].move->try_apply(problem, solution, scorer,
                                               temperature, random);
  }
};

}  // namespace annealing
