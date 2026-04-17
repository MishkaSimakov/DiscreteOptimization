#pragma once

#include <random>

#include "State.h"
#include "helpers/Random.h"

namespace facility {

struct OpenFacilityAction {
  size_t facility;
};

class OpenFacilityManager {
  std::default_random_engine random_;
  std::vector<size_t> customers_;

  size_t choose_facility(const SolutionState& state) {
    return state.closed[rnd::index(state.closed.size(), random_)];
  }

 public:
  explicit OpenFacilityManager(const Problem& problem)
      : customers_(problem.customers.size()) {
    std::iota(customers_.begin(), customers_.end(), 0);
  }

  OpenFacilityAction generate(const SolutionState& state) {
    return OpenFacilityAction{choose_facility(state)};
  }

  double get_gain(const SolutionState& state, OpenFacilityAction action,
                  double infeasibility_coef) {
    // use simple greedy algorithm to grab some customers to it

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
