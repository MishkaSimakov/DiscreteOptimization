#pragma once
#include <random>

#include "ProblemState.h"
#include "SolutionState.h"
#include "helpers/OnceAssigned.h"
#include "helpers/Random.h"

namespace tsp {

struct TwoOptAction {
  size_t t1;
  size_t t2;
  size_t t3;
  size_t t4;
};

class TwoOptActionManager {
 public:
  using Action = TwoOptAction;

 private:
  const ProblemState& state;
  std::default_random_engine random_;

 public:
  explicit TwoOptActionManager(const ProblemState& problem) : state(problem) {}

  TwoOptAction generate(const SolutionState& solution) {
    const size_t n = state.problem.points.size();

    const size_t t1 = rnd::index(n, random_);
    const size_t direction = rnd::index(2, random_);

    const size_t t2 =
        direction == 0 ? solution.tour.succ(t1) : solution.tour.pred(t1);

    size_t count = 0;

    for (const size_t t3 : state.candidates[t2]) {
      if (!solution.tour.is_neighbors(t3, t2)) {
        ++count;
      }
    }

    size_t chosen_t3_index = rnd::index(count, random_);
    OnceAssigned<size_t> chosen_t3;

    for (const size_t t3 : state.candidates[t2]) {
      if (solution.tour.is_neighbors(t3, t2)) {
        continue;
      }

      if (chosen_t3_index == 0) {
        chosen_t3 = t3;
        break;
      }

      --chosen_t3_index;
    }

    return TwoOptAction{
        .t1 = t1,
        .t2 = t2,
        .t3 = chosen_t3,
        .t4 = solution.tour.get_2opt_node(t1, t2, chosen_t3),
    };
  }

  annealing::ActionGain get_gain(const SolutionState& solution,
                                 TwoOptAction action) {
    const double score_gain = state.problem.get_distance(action.t1, action.t2) +
                              state.problem.get_distance(action.t3, action.t4) -
                              state.problem.get_distance(action.t1, action.t4) -
                              state.problem.get_distance(action.t2, action.t3);

    return annealing::ActionGain{
        .score = score_gain,
        .infeasibility = 0,
    };
  }

  void apply_action(SolutionState& state, TwoOptAction action) {
    state.tour.apply_2opt(action.t1, action.t2, action.t3);
  }
};

}  // namespace tsp
