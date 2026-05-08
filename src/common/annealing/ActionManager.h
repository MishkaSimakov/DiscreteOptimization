#pragma once
#include <utility>

#include "ActionGain.h"

namespace annealing {

template <typename T, typename ProblemState, typename SolutionState>
concept ActionManager =
    requires(T manager, const ProblemState& problem_state, SolutionState state,
             size_t changes_count, typename T::Action action) {
      T(problem_state);

      {
        manager.generate(std::as_const(state), changes_count)
      } -> std::same_as<std::optional<typename T::Action>>;
      {
        manager.get_gain(std::as_const(state), std::as_const(action))
      } -> std::same_as<ActionGain>;
      manager.apply_action(state, action);
    };

}  // namespace annealing
