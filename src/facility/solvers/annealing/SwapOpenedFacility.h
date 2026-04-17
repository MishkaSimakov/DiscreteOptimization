#pragma once

#include <random>

#include "State.h"
#include "helpers/Random.h"

namespace facility {

struct SwapOpenedFacilityAction {
  size_t opened;
  size_t closed;
};

class SwapOpenedFacilityManager {
  std::default_random_engine random_;

  size_t choose_opened_facility(const SolutionState& state) {
    return state.opened[rnd::index(state.opened.size(), random_)];
  }

  size_t choose_closed_facility(const SolutionState& state, size_t opened) {
    auto closed = state.closed;

    std::ranges::sort(closed, {}, [&](size_t i) {
      return distance_sqr(state.problem.facilities[i].position,
                          state.problem.facilities[opened].position);
    });

    for (const size_t facility : closed) {
      if (rnd::bernoulli(0.4, random_)) {
        return facility;
      }
    }

    return closed[0];
  }

 public:
  SwapOpenedFacilityAction generate(const SolutionState& state) {
    const size_t opened = choose_opened_facility(state);
    const size_t closed = choose_closed_facility(state, opened);

    return SwapOpenedFacilityAction{opened, closed};
  }

  std::pair<double, double> get_gain(const SolutionState& state,
                                     SwapOpenedFacilityAction action) {
    const Facility& opened = state.problem.facilities[action.opened];
    const Facility& closed = state.problem.facilities[action.closed];

    double gain = opened.cost - closed.cost;

    // distance gain
    for (size_t i = 0; i < state.problem.customers.size(); ++i) {
      const Customer& customer = state.problem.customers[i];

      if (state.solution.facility[i] == action.opened) {
        gain += distance(customer.position, opened.position) -
                distance(customer.position, closed.position);
      }
    }

    const double infeasibility_gain =
        std::max(state.demands[action.opened] - opened.capacity, 0.) -
        std::max(state.demands[action.opened] - closed.capacity, 0.);

    return {gain, infeasibility_gain};
  }

  void apply_action(SolutionState& state, SwapOpenedFacilityAction action) {
    const double demand = state.demands[action.opened];

    std::swap(state.demands[action.opened], state.demands[action.closed]);

    for (size_t& v : state.solution.facility) {
      if (v == action.opened) {
        v = action.closed;
      }
    }

    for (size_t& v : state.opened) {
      if (v == action.opened) {
        v = action.closed;
        break;
      }
    }

    for (size_t& v : state.closed) {
      if (v == action.closed) {
        v = action.opened;
        break;
      }
    }

    if (demand <= state.problem.facilities[action.opened].capacity !=
        demand <= state.problem.facilities[action.closed].capacity) {
      state.update_infeasible_customers();
    }
  }
};

}  // namespace facility
