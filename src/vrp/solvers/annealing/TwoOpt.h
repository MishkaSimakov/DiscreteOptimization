#pragma once

#include <random>

#include "ProblemState.h"
#include "SolutionState.h"
#include "common/annealing/ActionGain.h"
#include "helpers/OnceAssigned.h"
#include "helpers/Random.h"

namespace vrp {

struct TwoOptAction {
  size_t t1;
  size_t t2;
  size_t t3;
  size_t t4;
};

class TwoOptManager {
 public:
  using Action = TwoOptAction;

 private:
  const ProblemState& state_;
  std::default_random_engine random_;

 public:
  explicit TwoOptManager(const ProblemState& problem) : state_(problem) {
    static_assert(ProblemState::candidates_count > 2,
                  "If candidates count <= 2, then generate method can fail, "
                  "because all candidates are neighbours.");
  }

  std::optional<TwoOptAction> generate(const SolutionState& solution) {
    const size_t n = state_.problem.customers.size();

    const size_t t1 = rnd::index(n, random_);

    const size_t vehicle = solution.customers[t1].vehicle;
    const size_t t2 = solution.customers[t1].next;

    size_t count = 0;
    size_t current = n + vehicle;

    do {
      if (solution.customers[t2].next != current &&
          solution.customers[t2].prev != current && t2 != current) {
        ++count;
      }

      current = solution.customers[current].next;
    } while (current != n + vehicle);

    if (count == 0) {
      return std::nullopt;
    }

    OnceAssigned<size_t> t3;

    size_t t3_index = rnd::index(count, random_);
    current = n + vehicle;

    do {
      if (solution.customers[t2].next != current &&
          solution.customers[t2].prev != current && t2 != current) {
        if (t3_index == 0) {
          t3 = current;
          break;
        }

        --t3_index;
      }

      current = solution.customers[current].next;
    } while (current != n + vehicle);

    return TwoOptAction{
        .t1 = t1,
        .t2 = t2,
        .t3 = t3,
        .t4 = solution.customers[t3].prev,
    };
  }

  annealing::ActionGain get_gain(const SolutionState& solution,
                                 TwoOptAction action) {
    const double score_gain = state_.get_distance(action.t1, action.t2) +
                              state_.get_distance(action.t3, action.t4) -
                              state_.get_distance(action.t1, action.t4) -
                              state_.get_distance(action.t2, action.t3);

    return annealing::ActionGain{
        .score = score_gain,
        .infeasibility = 0,
    };
  }

  void apply_action(SolutionState& solution, TwoOptAction action) {
    // reverse fragment from t4 to t2
    size_t current = action.t4;
    const size_t end = solution.customers[action.t2].prev;

    do {
      const auto node = solution.customers[current];

      std::swap(solution.customers[current].next,
                solution.customers[current].prev);

      current = node.prev;
    } while (current != end);

    // update links between t1, t2, t3, t4
    solution.customers[action.t1].next = action.t4;
    solution.customers[action.t4].prev = action.t1;

    solution.customers[action.t2].next = action.t3;
    solution.customers[action.t3].prev = action.t2;
  }
};

}  // namespace vrp
