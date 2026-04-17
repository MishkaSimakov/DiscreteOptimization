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

  template <typename Manager>
  bool try_apply(Manager& manager, SolutionState& state) {
    std::uniform_real_distribution<double> prob(0, 1);

    const auto action = manager.generate(std::as_const(state));

    const double gain =
        manager.get_gain(std::as_const(state), action, infeasibility_coef_);

    // accept using simulated annealing algorithm
    if (gain > 0 || std::exp(gain / temperature_) > prob(random_)) {
      manager.apply_action(state, action);

      return true;
    }

    return false;
  }

 public:
  explicit SimulatedAnnealing(const Problem& problem, timing::Deadline deadline)
      : problem(problem), deadline(deadline) {}

  Solution solve(const Solution& initial_solution) {
    constexpr double relative_start_temperature = 1e-4;
    constexpr double delta = 0.01 * relative_start_temperature;
    constexpr size_t iterations_per_temperature = 10;

    SolutionState state(problem, initial_solution);
    double current_cost = get_score(problem, initial_solution);

    ChangeCustomerFacilityManager change_customer_facility_manager;
    SwapOpenedFacilityManager swap_opened_facility_manager;

    Solution best_solution = initial_solution;
    double best_cost = current_cost;

    size_t iteration = 0;

    temperature_ = relative_start_temperature * current_cost;
    infeasibility_coef_ = 50;

    while (temperature_ > 0) {
      bool changed;

      if (rnd::bernoulli(0.9, random_)) {
        changed = try_apply(change_customer_facility_manager, state);
      } else {
        changed = try_apply(swap_opened_facility_manager, state);
      }

      if (changed) {
        current_cost = get_score(problem, state.solution);
        const double infeasibility = get_infeasibility(problem, state.solution);

        if (current_cost < best_cost && infeasibility == 0) {
          best_cost = current_cost;
          best_solution = state.solution;
        }
      }

      if ((iteration + 1) % iterations_per_temperature == 0) {
        temperature_ -= delta;
        infeasibility_coef_ /= 0.99;
      }
      ++iteration;

      if (deadline.is_over()) {
        break;
      }
    }

    return best_solution;
  }
};

}  // namespace facility
