#pragma once

#include <utils/Accumulators.h>
#include <utils/Logging.h>

#include <cmath>
#include <functional>
#include <iostream>
#include <random>
#include <ranges>
#include <stdexcept>
#include <tuple>

#include "ActionManager.h"
#include "CoolingProcess.h"
#include "InfeasibilityController.h"
#include "helpers/Random.h"
#include "helpers/Time.h"

namespace annealing {

struct SimulatedAnnealingConfig {
  bool log_best;
  bool log_iteration_end;
};

// Solves minimization problem with constraints.
// Can operate in infeasible solution spaces.
//  - ProblemState should store the Problem and precalculated immutable problem
// information.
//  - SolutionState should store solution as well as some calculated
// characteristics of it, that may be updated after action is applied.
template <typename Problem, typename ProblemState, typename Solution,
          typename SolutionState, CoolingProcess C>
class SimulatedAnnealing {
  const ProblemState problem_state;
  const timing::Deadline deadline;
  const SimulatedAnnealingConfig config;

  std::default_random_engine random_;

  struct SolverStateDTO {
    SolutionState& solution;
    double temperature;
    double infeasibility_penalty;
    std::default_random_engine& random;
  };

  struct ActionType {
    std::string name;
    double weight;

    // Samples random action of this type, applies it with Simulated Annealing
    // probability model.
    std::function<std::optional<ActionGain>(SolverStateDTO)> try_apply;

    // Samples random action of this type, and returns its gain.
    std::function<ActionGain(const SolutionState&)> get_gain;
  };

  std::vector<ActionType> actions_;

  struct ActionTypeStats {
    size_t proposed_transitions{0};
    size_t accepted_transitions{0};

    double get_acceptance_rate() const {
      return static_cast<double>(accepted_transitions) /
             static_cast<double>(proposed_transitions);
    }
  };

  struct BestSolution {
    Solution solution;
    double score;
    double infeasibility;
  };

  // Tries to apply given action to current solution state.
  // If successfully applied, returns gain and infeasibility gain. Otherwise,
  // returns std::nullopt.
  template <ActionManager<ProblemState, SolutionState> M>
  static std::optional<ActionGain> try_apply(M& manager, SolverStateDTO state) {
    std::uniform_real_distribution<double> prob(0, 1);

    const std::optional<typename M::Action> action =
        manager.generate(std::as_const(state.solution));

    if (!action) {
      return std::nullopt;
    }

    const ActionGain gain =
        manager.get_gain(std::as_const(state.solution), std::as_const(*action));

    const double combined_gain =
        gain.score + gain.infeasibility * state.infeasibility_penalty;

    // accept using simulated annealing algorithm
    if (combined_gain > 0 ||
        std::exp(combined_gain / state.temperature) > prob(state.random)) {
      manager.apply_action(state.solution, std::move(*action));

      return gain;
    }

    return std::nullopt;
  }

  template <ActionManager<ProblemState, SolutionState> M>
  static ActionGain get_gain(M& manager, const SolutionState& state) {
    const std::optional<typename M::Action> action = manager.generate(state);

    if (!action) {
      return ActionGain{0, 0};
    }

    return manager.get_gain(state, *action);
  }

  double get_total_actions_weight() const {
    double result = 0;

    for (const ActionType& action : actions_) {
      result += action.weight;
    }

    return result;
  }

  // Returns index of the chosen action type.
  size_t get_random_action_type() {
    assert(!actions_.empty());

    const double total_weight = get_total_actions_weight();

    std::uniform_real_distribution<double> prob(0, 1);
    double value = prob(random_) * total_weight;

    for (size_t i = 0; i < actions_.size(); ++i) {
      value -= actions_[i].weight;

      if (value <= 0) {
        return i;
      }
    }

    return actions_.size() - 1;
  }

  void assert_score_validity([[maybe_unused]] const SolutionState& state,
                             [[maybe_unused]] double score,
                             [[maybe_unused]] double infeasibility) {
#ifndef NDEBUG
    const double real_score =
        get_score(problem_state.get_problem(), state.get_solution());
    assert(std::abs(real_score - score) < 1e-3);

    const double real_infeasibility =
        get_infeasibility(problem_state.get_problem(), state.get_solution());
    assert(std::abs(real_infeasibility - infeasibility) < 1e-3);
#endif
  }

  void log_new_best(const BestSolution& best) const {
    if (!config.log_best) {
      return;
    }

    if (best.infeasibility > 0) {
      std::println("  [.] new best: {} (infeasibility = {})", best.score,
                   best.infeasibility);
    } else {
      std::println("  [!] new best: {} (feasible)", best.score);
    }
  }

 public:
  explicit SimulatedAnnealing(const Problem& problem, timing::Deadline deadline,
                              SimulatedAnnealingConfig config)
      : problem_state(problem), deadline(deadline), config(config) {}

  // non copyable
  SimulatedAnnealing(const SimulatedAnnealing&) = delete;
  SimulatedAnnealing& operator=(const SimulatedAnnealing&) = delete;

  // non movable
  SimulatedAnnealing(SimulatedAnnealing&&) = delete;
  SimulatedAnnealing& operator=(SimulatedAnnealing&&) = delete;

  template <ActionManager<ProblemState, SolutionState> M>
  void add(const std::string& name, double weight) {
    ActionType action{
        .name = name,
        .weight = weight,
        .try_apply =
            [manager = M(problem_state)](SolverStateDTO state) mutable {
              return try_apply(manager, state);
            },
        .get_gain =
            [manager = M(problem_state)](const SolutionState& state) mutable {
              return get_gain(manager, state);
            },
    };

    actions_.push_back(std::move(action));
  }

  // Returns the best found solution. Solutions are compared first by
  // infeasibility, then by score.
  // Note: returned solution may not be feasible.
  Solution solve(const Solution& initial_solution, C cooling,
                 double infeasibility_penalty) {
    if (actions_.empty()) {
      throw std::runtime_error("No actions are available.");
    }

    std::vector<ActionTypeStats> actions_stats(actions_.size());

    SolutionState state(std::as_const(problem_state), initial_solution);
    double current_score =
        get_score(problem_state.get_problem(), initial_solution);
    double current_infeasibility =
        get_infeasibility(problem_state.get_problem(), initial_solution);

    BestSolution best{
        .solution = initial_solution,
        .score = current_score,
        .infeasibility = current_infeasibility,
    };

    InfeasibilityController infeasibility(infeasibility_penalty);

    size_t iterations_per_temperature = 100;
    size_t iterations_until_change = iterations_per_temperature;

    using Duration = std::chrono::duration<double, std::nano>;
    using Clock = std::chrono::steady_clock;

    auto iteration_start = Clock::now();

    while (true) {
      const size_t chosen_action = get_random_action_type();

      SolverStateDTO state_dto{
          .solution = state,
          .temperature = cooling.get_temperature(),
          .infeasibility_penalty = infeasibility.get_penalty(),
          .random = random_,
      };
      const std::optional<ActionGain> gain =
          actions_[chosen_action].try_apply(state_dto);

      ++actions_stats[chosen_action].proposed_transitions;
      if (gain) {
        ++actions_stats[chosen_action].accepted_transitions;

        current_score -= gain->score;
        current_infeasibility -= gain->infeasibility;

        infeasibility.update(current_infeasibility, cooling.get_temperature());

        // Score and infeasibility are updated incrementally.
        // If any action contains error in gain calculation, all further
        // actions will with incorrect score.
        // In debug, we are better off checking that the score remains valid.
        assert_score_validity(state, current_score, current_infeasibility);

        if (current_infeasibility < best.infeasibility ||
            current_infeasibility == best.infeasibility &&
                current_score < best.score - 1e-5) {
          best = BestSolution{
              .solution = state.get_solution(),
              .score = current_score,
              .infeasibility = current_infeasibility,
          };

          log_new_best(best);
        }
      }

      if (iterations_until_change == 0) {
        const auto remaining_temperatures = cooling.advance();

        if (!remaining_temperatures) {
          break;
        }

        const auto now = Clock::now();

        const Duration remaining_time =
            std::chrono::duration_cast<Duration>(deadline.remaining_time());
        const Duration average_iteration_time =
            Duration(now - iteration_start) / iterations_per_temperature;

        if (remaining_time < std::chrono::milliseconds{100}) {
          break;
        }

        iterations_until_change = iterations_per_temperature =
            remaining_time / average_iteration_time /
            (*remaining_temperatures + 1);

        iteration_start = now;

        if (config.log_iteration_end) {
          std::println(
              "  # iterations = {} (average itr time = {}), score = {}, "
              "inf = "
              "{}, coef = "
              "{}, T = {}",
              iterations_until_change, average_iteration_time, current_score,
              current_infeasibility, infeasibility.get_penalty(),
              cooling.get_temperature());

          std::println("  iteration acceptance rates:");
          for (size_t i = 0; i < actions_.size(); ++i) {
            std::println("  - {}: {} (accepted: {}, proposed: {})",
                         actions_[i].name,
                         actions_stats[i].get_acceptance_rate(),
                         actions_stats[i].accepted_transitions,
                         actions_stats[i].proposed_transitions);
          }
        }

        // reset actions statistics
        actions_stats = std::vector<ActionTypeStats>(actions_.size());
      }

      --iterations_until_change;
    }

    return best.solution;
  }

  // Randomly samples actions, and measures gain by applying them to @solution.
  // Returns temperature for which expected value of acceptance rate is
  // @desired_rate. Ignores infeasibility gain.
  double estimate_start_temperature(const size_t samples,
                                    const double desired_rate,
                                    const Solution& solution) {
    constexpr double tolerance = 1e-5;

    if (actions_.empty()) {
      throw std::runtime_error("No actions are available.");
    }

    SolutionState state(std::as_const(problem_state), solution);

    std::vector<double> alphas(samples);
    size_t positive_gain_count = 0;

    for (size_t i = 0; i < samples; ++i) {
      const size_t action_type = get_random_action_type();

      const ActionGain gain = actions_[action_type].get_gain(state);

      if (gain.score > -tolerance) {
        ++positive_gain_count;
      }

      alphas[i] = std::min(gain.score, 0.);
    }

    // If more than half of the gains were non-negative, then any temperature
    // would suffice
    if (static_cast<double>(positive_gain_count) >=
        static_cast<double>(samples) * desired_rate) {
      std::cerr << "Warning: too many samples gave non-negative gain. This "
                   "smells fishy."
                << std::endl;
      return 1.;
    }

    // use binary search to find starting temperature
    double left = 0;
    double right = 1e10;

    while (right - left > tolerance) {
      double middle = (left + right) / 2;
      double acceptance = 0;

      for (size_t i = 0; i < samples; ++i) {
        acceptance += std::exp(alphas[i] / middle);
      }

      acceptance /= static_cast<double>(samples);

      if (acceptance > desired_rate) {
        // decrease temperature
        right = middle;
      } else {
        // increase temperature
        left = middle;
      }
    }

    return (left + right) / 2;
  }
};

}  // namespace annealing
