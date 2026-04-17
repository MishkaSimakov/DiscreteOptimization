#pragma once

#include <cmath>
#include <iostream>
#include <random>

#include "ChangeCustomerFacility.h"
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

  // Tries to apply given action to current_solution.
  // If successfully applied, returns gain. Otherwise, returns std::nullopt.
  template <typename Manager>
  std::optional<double> try_apply(Manager& manager, SolutionState& state) {
    std::uniform_real_distribution<double> prob(0, 1);

    const auto action = manager.generate(std::as_const(state));

    const auto [score_gain, infeasibility_gain] =
        manager.get_gain(std::as_const(state), action);

    const double gain = score_gain + infeasibility_gain * infeasibility_coef_;

    // accept using simulated annealing algorithm
    if (gain > 0 || std::exp(gain / temperature_) > prob(random_)) {
      manager.apply_action(state, action);

      return score_gain;
    }

    return std::nullopt;
  }

 public:
  explicit SimulatedAnnealing(const Problem& problem, timing::Deadline deadline)
      : problem(problem), deadline(deadline) {}

  Solution solve(const Solution& initial_solution) {
    constexpr double relative_start_temperature = 1e-4;

    SolutionState state(problem, initial_solution);
    double current_cost = get_score(problem, initial_solution);

    ChangeCustomerFacilityManager change_customer_facility_manager;
    SwapOpenedFacilityManager swap_opened_facility_manager;

    Solution best_solution = initial_solution;
    double best_cost = current_cost;

    temperature_ = relative_start_temperature * current_cost;
    infeasibility_coef_ = 50;

    const double delta = 0.01 * temperature_;

    size_t iterations_until_change = 10;

    // time in nanoseconds
    ArithmeticMean<double> average_iteration_time;

    // statistics
    size_t change_customer_facility_count = 0;
    size_t change_customer_facility_successful_count = 0;

    size_t swap_opened_facility_count = 0;
    size_t swap_opened_facility_successful_count = 0;

    while (temperature_ > 1e-20) {
      auto iteration_duration = timing::timeit([&] {
        std::optional<double> gain;

        if (rnd::bernoulli(0.9, random_)) {
          gain = try_apply(change_customer_facility_manager, state);

          ++change_customer_facility_count;
          if (gain) {
            ++change_customer_facility_successful_count;
          }
        } else {
          gain = try_apply(swap_opened_facility_manager, state);

          ++swap_opened_facility_count;
          if (gain) {
            ++swap_opened_facility_successful_count;
          }
        }

        if (gain) {
          current_cost -= *gain;

          const double infeasibility =
              get_infeasibility(problem, state.solution);

          if (current_cost < best_cost && infeasibility == 0) {
            best_cost = current_cost;
            best_solution = state.solution;
          }
        }
      });

      average_iteration_time.record(
          static_cast<double>(iteration_duration.count()));

      if (iterations_until_change == 0) {
        temperature_ -= delta;
        infeasibility_coef_ /= 0.99;

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

        std::println("  # iterations = {}", iterations_until_change);
      }

      --iterations_until_change;

      if (deadline.is_over()) {
        break;
      }
    }

    std::println("  change_customer_facility: {} (out of {})",
                 change_customer_facility_successful_count,
                 change_customer_facility_count);
    std::println("  swap_opened_facility: {} (out of {})",
                 swap_opened_facility_successful_count,
                 swap_opened_facility_count);
    std::println("  T_end = {}, delta = {}", temperature_, delta);

    return best_solution;
  }
};

}  // namespace facility
