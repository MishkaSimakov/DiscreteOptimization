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

#include "Acceptance.h"
#include "ActionManager.h"
#include "ActionManagerBox.h"
#include "CoolingProcess.h"
#include "InfeasibilityController.h"
#include "LoggingContext.h"
#include "ScoredSolution.h"
#include "helpers/Random.h"
#include "helpers/Time.h"

namespace annealing {

struct SimulatedAnnealingConfig {
  bool verify_gain;
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
  struct ActionType {
    std::string name;
    double weight;

    std::unique_ptr<ActionManagerBox<SolutionState>> box;
  };

  const ProblemState problem_state;
  const SimulatedAnnealingConfig config;

  std::default_random_engine random_;
  std::vector<ActionType> actions_;

  using Logger =
      std::function<void(const LoggingContext<ProblemState, SolutionState>&)>;
  Logger log_best;
  Logger log_state;

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

  void assert_score_validity(
      [[maybe_unused]] const ScoredSolution<SolutionState>& state) {
    if (config.verify_gain) {
      const double real_score =
          get_score(problem_state.get_problem(), state.solution.get_solution());
      if (std::abs(real_score - state.score) > 1e-3) {
        throw std::runtime_error("Score gain is incorrect.");
      }

      const double real_infeasibility = get_infeasibility(
          problem_state.get_problem(), state.solution.get_solution());
      if (std::abs(real_infeasibility - state.infeasibility) > 1e-3) {
        throw std::runtime_error("Infeasibility gain is incorrect.");
      }
    }
  }

  static double get_spent_ratio(std::chrono::nanoseconds spent,
                                std::chrono::nanoseconds total) {
    return std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(
               spent) /
           total;
  }

 public:
  explicit SimulatedAnnealing(const Problem& problem,
                              SimulatedAnnealingConfig config)
      : problem_state(problem), config(config) {}

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
        .box = std::make_unique<
            ActionManagerBoxImpl<M, ProblemState, SolutionState>>(
            problem_state),
    };

    actions_.push_back(std::move(action));
  }

  void set_log_best(Logger logger) { log_best = logger; }
  void set_log_state(Logger logger) { log_state = logger; }

  // Returns the best found solution. Solutions are compared first by
  // infeasibility, then by score.
  // Note: returned solution may not be feasible.
  Solution solve(const Solution& initial_solution, C cooling,
                 double infeasibility_penalty,
                 const std::chrono::nanoseconds duration) {
    if (actions_.empty()) {
      throw std::runtime_error("No actions are available.");
    }

    for (const auto& action : actions_) {
      action.box->reset();
    }

    std::unordered_map<std::string, ActionAcceptance> acceptances;

    ScoredSolution<SolutionState> current{
        .solution =
            SolutionState(std::as_const(problem_state), initial_solution),
        .score = get_score(problem_state.get_problem(), initial_solution),
        .infeasibility =
            get_infeasibility(problem_state.get_problem(), initial_solution),
    };

    ScoredSolution<SolutionState> best = current;

    // May be used to implement taboo list inside actions
    size_t changes_count = 0;

    InfeasibilityController infeasibility(infeasibility_penalty);

    constexpr size_t temperature_update_period = 100;
    size_t iteration = 0;
    double temperature = cooling.get_temperature(0);

    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();

    while (true) {
      const size_t chosen_action = get_random_action_type();

      InternalStateDTO state_dto{
          .solution = current.solution,
          .temperature = temperature,
          .infeasibility_penalty = infeasibility.get_penalty(),
          .random = random_,
          .changes_count = changes_count,
      };
      const std::optional<ActionGain> gain =
          actions_[chosen_action].box->try_apply(state_dto);

      ++acceptances[actions_[chosen_action].name].proposed_transitions;
      if (gain) {
        ++changes_count;
        ++acceptances[actions_[chosen_action].name].accepted_transitions;

        current.score -= gain->score;
        current.infeasibility -= gain->infeasibility;

        infeasibility.update(current.infeasibility, temperature);

        // Score and infeasibility are updated incrementally.
        // If any action contains error in gain calculation, all further
        // actions will with incorrect score.
        // In debug, we are better off checking that the score remains valid.
        assert_score_validity(current);

        if (current < best) {
          best = current;

          if (log_best) {
            LoggingContext<ProblemState, SolutionState> context{
                .problem = problem_state,
                .current = current,
                .best = best,
                .temperature = temperature,
                .infeasibility_penalty = infeasibility_penalty,
                .changes_count = changes_count,
                .acceptances = acceptances,
            };

            log_best(context);
          }
        }
      }

      if ((iteration + 1) % temperature_update_period == 0) {
        const auto now = Clock::now();
        const double ratio = get_spent_ratio(now - start, duration);

        if (ratio > 1.) {
          break;
        }

        temperature = cooling.get_temperature(ratio);
      }

      ++iteration;
    }

    return best.solution.get_solution();
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

      const ActionGain gain = actions_[action_type].box->get_gain(state);

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
