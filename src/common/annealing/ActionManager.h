#pragma once
#include <utility>

#include "ActionGain.h"

namespace annealing {

template <typename T, typename Problem, typename SolutionState>
concept ActionManager =
    requires(T manager, const Problem& problem, SolutionState state,
             typename T::Action action) {
      T(problem);

      {
        manager.generate(std::as_const(state))
      } -> std::same_as<typename T::Action>;
      {
        manager.get_gain(std::as_const(state), std::as_const(action))
      } -> std::same_as<ActionGain>;
      manager.apply_action(state, action);
    };

}  // namespace annealing
