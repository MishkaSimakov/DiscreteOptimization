#pragma once

#pragma once

#include <utils/Accumulators.h>

#include <cmath>
#include <functional>
#include <iostream>
#include <random>
#include <ranges>
#include <tuple>

#include "ActionManager.h"
#include "helpers/Random.h"
#include "helpers/Time.h"

namespace annealing {

// Solves minimization problem with constraints.
// Can operate in infeasible solution spaces.
template <typename Problem, typename Solution, typename SolutionState,
          typename ClimateControl>
class SimulatedAnnealing {
  const Problem& problem;
  const timing::Deadline deadline;

  std::default_random_engine random_;

  double infeasibility_coef_;
  ClimateControl climate_;

  struct ActionType {
    std::string name;
    double weight;
    std::function<std::optional<ActionGain>(SolutionState&)> try_apply;
  };

  std::vector<ActionType> actions_;

  // Tries to apply given action to current solution state.
  // If successfully applied, returns gain and infeasibility gain. Otherwise,
  // returns std::nullopt.
  template <ActionManager<Problem, SolutionState> M>
  std::optional<ActionGain> try_apply(M& manager, SolutionState& state) {
    std::uniform_real_distribution<double> prob(0, 1);

    const typename M::Action action = manager.generate(std::as_const(state));

    const ActionGain gain =
        manager.get_gain(std::as_const(state), std::as_const(action));

    const double combined_gain =
        gain.score_gain + gain.infeasibility_gain * infeasibility_coef_;

    // accept using simulated annealing algorithm
    if (combined_gain > 0 ||
        std::exp(combined_gain / climate_.get_temperature()) > prob(random_)) {
      manager.apply_action(state, std::move(action));
      ++state.changes_count;

      return gain;
    }

    return std::nullopt;
  }

  double get_total_actions_weight() const {
    double result = 0;

    for (const double weight : actions_ | std::views::elements<1>) {
      result += weight;
    }

    return result;
  }

  // Returns index of the chosen action.
  size_t get_random_action() {
    const double total_weight = get_total_actions_weight();

    std::uniform_real_distribution<double> prob(0, 1);
    double value = prob(random_) * total_weight;

    for (size_t i = 0; i < actions_.size(); ++i) {
      value -= actions_[i].weight;

      if (value <= 0) {
        return i;
      }
    }

    std::unreachable();
  }

 public:
  explicit SimulatedAnnealing(const Problem& problem, timing::Deadline deadline)
      : problem(problem), deadline(deadline) {}

  template <ActionManager<Problem, SolutionState> M>
  void add(const std::string& name, double weight) {
    ActionType action{
        .name = name,
        .weight = weight,
        .try_apply =
            [&, manager = M(problem)](SolutionState& state) mutable {
              return try_apply(manager, state);
            },
    };

    actions_.push_back(std::move(action));
  }

  Solution solve(const Solution& initial_solution) {
    if (actions_.empty()) {
      throw std::runtime_error("No actions are available.");
    }

    SolutionState state(problem, initial_solution);
    double current_cost = get_score(problem, initial_solution);
    double current_infeasibility = get_infeasibility(problem, initial_solution);

    Solution best_solution = initial_solution;
    double best_cost = current_cost;

    constexpr double base_infeasibility_coef_value = 50;
    infeasibility_coef_ = base_infeasibility_coef_value;

    double integral_infeasibility_component = 0;

    size_t iterations_until_change = 10;

    // time in nanoseconds
    ArithmeticMean<double> average_iteration_time;

    // for each action name stores (total count, successful count)
    std::vector<std::pair<size_t, size_t>> actions_stats(actions_.size(),
                                                         {0, 0});

    while (temperature_ > 1e-20) {
      auto iteration_duration = timing::timeit([&] {
        const size_t chosen_action = get_random_action();

        std::optional<ActionGain> gain =
            actions_[chosen_action].try_apply(state);

        ++actions_stats[chosen_action].first;
        if (gain) {
          ++actions_stats[chosen_action].second;

          current_cost -= gain->score_gain;
          current_infeasibility -= gain->infeasibility_gain;

          if (current_infeasibility == 0) {
            integral_infeasibility_component = 0;
          } else {
            integral_infeasibility_component += current_infeasibility;
          }

          infeasibility_coef_ =
              base_infeasibility_coef_value *
              std::exp(0.001 * (current_infeasibility +
                                integral_infeasibility_component));

          infeasibility_coef_ = std::min(1e6, infeasibility_coef_);

          assert(std::abs(get_score(problem, state.solution) - current_cost) <
                 1e-3);
          assert(std::abs(get_infeasibility(problem, state.solution) -
                          current_infeasibility) < 1e-3);

          if (current_cost < best_cost && current_infeasibility == 0) {
            std::println("  [!] new best: {}", current_cost);

            best_cost = current_cost;
            best_solution = state.solution;
          }
        }
      });

      average_iteration_time.record(
          static_cast<double>(iteration_duration.count()));

      if (iterations_until_change == 0) {
        climate_.advance();

        const size_t remaining_temperature_changes =
            static_cast<size_t>(temperature_ / delta);

        const size_t remaining_nanoseconds =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                deadline.remaining_time())
                .count();

        if (remaining_nanoseconds == 0 || remaining_temperature_changes == 0) {
          break;
        }

        const size_t nanoseconds_per_iteration =
            static_cast<size_t>(average_iteration_time.mean());

        iterations_until_change = remaining_nanoseconds /
                                  nanoseconds_per_iteration /
                                  remaining_temperature_changes;

        std::println(
            "  # iterations = {} (average itr time = {} ns), score = {}, inf = "
            "{}, coef = "
            "{}, T = {}, opened count = {}",
            iterations_until_change, average_iteration_time.mean(),
            current_cost, current_infeasibility, infeasibility_coef_,
            temperature_, state.opened.size());
      }

      --iterations_until_change;

      if (deadline.is_over()) {
        break;
      }
    }

    std::println("  actions stats:");
    for (const auto& name : actions_ | std::views::elements<0>) {
      std::println("  {}: {} (out of {})", name, actions_stats[name].second,
                   actions_stats[name].first);
    }
    std::println("  T_end = {}, delta = {}", temperature_, delta);

    return best_solution;
  }
};

}  // namespace annealing
