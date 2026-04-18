#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <random>
#include <tuple>

#include "ChangeCustomerFacility.h"
#include "OpenFacility.h"
#include "SwapOpenedFacility.h"
#include "facility/Evaluator.h"
#include "facility/Types.h"
#include "helpers/Hashers.h"
#include "helpers/Random.h"
#include "helpers/Time.h"

namespace facility {

class SimulatedAnnealing {
  const Problem& problem;
  const timing::Deadline deadline;

  std::default_random_engine random_;

  double temperature_;
  double infeasibility_coef_;

  std::vector<std::tuple<
      std::string, double,
      std::function<std::optional<std::pair<double, double>>(SolutionState&)>>>
      actions_;

  // Tries to apply given action to current_solution.
  // If successfully applied, returns gain and infeasibility gain. Otherwise,
  // returns std::nullopt.
  template <typename Manager>
  std::optional<std::pair<double, double>> try_apply(Manager& manager,
                                                     SolutionState& state) {
    std::uniform_real_distribution<double> prob(0, 1);

    const auto action = manager.generate(std::as_const(state));

    const auto [score_gain, infeasibility_gain] =
        manager.get_gain(std::as_const(state), action);

    const double gain = score_gain + infeasibility_gain * infeasibility_coef_;

    // accept using simulated annealing algorithm
    if (gain > 0 || std::exp(gain / temperature_) > prob(random_)) {
      manager.apply_action(state, action);

      return std::pair{score_gain, infeasibility_gain};
    }

    return std::nullopt;
  }

 public:
  explicit SimulatedAnnealing(const Problem& problem, timing::Deadline deadline)
      : problem(problem), deadline(deadline) {}

  template <typename Manager>
  void add(const std::string& name, double probability) {
    actions_.emplace_back(
        name, probability,
        [&, manager = Manager(problem)](SolutionState& state) mutable {
          return try_apply(manager, state);
        });
  }

  Solution solve(const Solution& initial_solution) {
    std::uniform_real_distribution<double> prob(0, 1);

    // 1e-3 \approx 8'930'000
    // 1e-4 \approx 8'920'000
    // 1e-5 \approx 9'240'000
    constexpr double relative_start_temperature = 7 * 1e-5;

    SolutionState state(problem, initial_solution);
    double current_cost = get_score(problem, initial_solution);
    double current_infeasibility = get_infeasibility(problem, initial_solution);

    Solution best_solution = initial_solution;
    double best_cost = current_cost;

    temperature_ = relative_start_temperature * current_cost;
    constexpr double base_infeasibility_coef_value = 50;
    infeasibility_coef_ = base_infeasibility_coef_value;

    double integral_infeasibility_component = 0;

    const double delta = 0.01 * temperature_;

    size_t iterations_until_change = 10;

    // time in nanoseconds
    ArithmeticMean<double> average_iteration_time;

    // for each action name stores (total count, successful count)
    std::unordered_map<std::string, std::pair<size_t, size_t>> actions_stats;
    for (const auto& name : actions_ | std::views::elements<0>) {
      actions_stats[name] = {0, 0};
    }

    // simple sanity check
    double total_prob = 0;
    for (const double action_prob : actions_ | std::views::elements<1>) {
      total_prob += action_prob;
    }

    assert(std::abs(total_prob - 1) < 1e-10);

    while (temperature_ > 1e-20) {
      auto iteration_duration = timing::timeit([&] {
        std::optional<std::pair<double, double>> gain;

        auto value = prob(random_);
        for (const auto& [name, probability, apply] : actions_) {
          value -= probability;

          if (value <= 0) {
            gain = apply(state);

            auto& stats = actions_stats[name];
            ++stats.first;

            if (gain) {
              ++stats.second;
            }

            break;
          }
        }

        if (gain) {
          current_cost -= gain->first;
          current_infeasibility -= gain->second;

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
        temperature_ -= delta;

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

}  // namespace facility

// 8980674.58538693
