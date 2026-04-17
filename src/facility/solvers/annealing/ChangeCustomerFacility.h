#pragma once

#include <random>

#include "State.h"
#include "helpers/Random.h"

namespace facility {

struct ChangeCustomerFacilityAction {
  size_t customer;
  size_t facility;
};

class ChangeCustomerFacilityManager {
  std::default_random_engine random_;

  size_t choose_customer(const SolutionState& state) {
    if (rnd::bernoulli(0.5, random_) && !state.infeasible_customers.empty()) {
      return state.infeasible_customers[rnd::index(
          state.infeasible_customers.size(), random_)];
    }

    return rnd::index(state.problem.customers.size(), random_);
  }

  size_t choose_facility(const SolutionState& state, size_t customer) {
    auto opened = state.opened;

    std::ranges::sort(opened, {}, [&](size_t facility) {
      if (facility == state.solution.facility[customer]) {
        return 1e10;
      }

      return distance(state.problem.customers[customer].position,
                      state.problem.facilities[facility].position);
    });

    for (const size_t facility : opened) {
      if (facility == state.solution.facility[customer]) {
        break;
      }

      if (rnd::bernoulli(0.4, random_)) {
        return facility;
      }
    }

    return opened[0];
  }

 public:
  ChangeCustomerFacilityAction generate(const SolutionState& state) {
    const size_t customer = choose_customer(state);
    const size_t facility = choose_facility(state, customer);

    return ChangeCustomerFacilityAction{customer, facility};
  }

  double get_gain(const SolutionState& state,
                  ChangeCustomerFacilityAction action,
                  double infeasibility_coef) {
    const size_t f0 = state.solution.facility[action.customer];
    const size_t f1 = action.facility;

    const Customer& customer = state.problem.customers[action.customer];

    // recalculate demands (f0 changed to f1)
    const double new_demand_f0 = state.demands[f0] - customer.demand;
    const double new_demand_f1 = state.demands[f1] + customer.demand;

    double distance_gain =
        distance(customer.position, state.problem.facilities[f0].position) -
        distance(customer.position, state.problem.facilities[f1].position);

    const double capacity_f0 = state.problem.facilities[f0].capacity;
    const double capacity_f1 = state.problem.facilities[f1].capacity;

    const double infeasibility_gain =
        std::max(state.demands[f0] - capacity_f0, 0.) -
        std::max(new_demand_f0 - capacity_f0, 0.) +
        std::max(state.demands[f1] - capacity_f1, 0.) -
        std::max(new_demand_f1 - capacity_f1, 0.);

    return distance_gain + infeasibility_gain * infeasibility_coef;
  }

  void apply_action(SolutionState& state, ChangeCustomerFacilityAction action) {
    const size_t f0 = state.solution.facility[action.customer];
    const size_t f1 = action.facility;

    const double old_demand_f0 = state.demands[f0];
    const double old_demand_f1 = state.demands[f1];

    const double new_demand_f0 =
        state.demands[f0] - state.problem.customers[action.customer].demand;
    const double new_demand_f1 =
        state.demands[f1] + state.problem.customers[action.customer].demand;

    state.solution.facility[action.customer] = f1;
    state.demands[f0] = new_demand_f0;
    state.demands[f1] = new_demand_f1;

    if (old_demand_f0 <= state.problem.facilities[f0].capacity !=
            new_demand_f0 <= state.problem.facilities[f0].capacity ||
        old_demand_f1 <= state.problem.facilities[f1].capacity !=
            new_demand_f1 <= state.problem.facilities[f1].capacity) {
      state.infeasible_customers.clear();

      for (size_t i = 0; i < state.problem.customers.size(); ++i) {
        const size_t f = state.solution.facility[i];

        if (state.demands[f] > state.problem.facilities[f].capacity) {
          state.infeasible_customers.push_back(i);
        }
      }
    }
  }
};

}  // namespace facility
