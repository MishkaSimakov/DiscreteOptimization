#pragma once

#include <optional>
#include <random>

#include "ActionGain.h"
#include "ActionManager.h"

namespace annealing {

template <typename Solution>
class ActionManagerBox {
 public:
  // Samples random action of this type, applies it with Simulated Annealing
  // probability model.
  virtual std::optional<ActionGain> try_apply(
      InternalStateDTO<Solution> solution) = 0;

  // Samples random action of this type, and returns its gain.
  virtual ActionGain get_gain(const Solution& solution) = 0;

  virtual void reset() = 0;

  virtual ~ActionManagerBox() = default;
};

template <typename M, typename Problem, typename Solution>
  requires(ActionManager<M, Problem, Solution>)
class ActionManagerBoxImpl final : public ActionManagerBox<Solution> {
  // Optional used for manual lifetime control
  std::optional<M> manager_;

  const Problem& problem_;

 public:
  explicit ActionManagerBoxImpl(const Problem& problem)
      : manager_(problem), problem_(problem) {}

  // Tries to apply given action to current solution state.
  // If successfully applied, returns gain and infeasibility gain. Otherwise,
  // returns std::nullopt.
  std::optional<ActionGain> try_apply(
      InternalStateDTO<Solution> state) override {
    std::uniform_real_distribution<double> prob(0, 1);

    const SolverStateDTO shared_state{.changes_count = state.changes_count};

    const std::optional<typename M::Action> action =
        manager_->generate(std::as_const(state.solution), shared_state);

    if (!action) {
      return std::nullopt;
    }

    const ActionGain gain = manager_->get_gain(std::as_const(state.solution),
                                               std::as_const(*action));

    const double combined_gain =
        gain.score + gain.infeasibility * state.infeasibility_penalty;

    // accept using simulated annealing algorithm
    if (combined_gain > 0 ||
        std::exp(combined_gain / state.temperature) > prob(state.random)) {
      manager_->apply_action(state.solution, std::move(*action), shared_state);

      return gain;
    }

    return std::nullopt;
  }

  ActionGain get_gain(const Solution& solution) override {
    constexpr SolverStateDTO shared_state{.changes_count = 0};

    const std::optional<typename M::Action> action =
        manager_->generate(solution, shared_state);

    if (!action) {
      return ActionGain{0, 0};
    }

    return manager_->get_gain(solution, *action);
  }

  void reset() override {
    manager_.reset();
    manager_.emplace(problem_);
  }
};

}  // namespace annealing
