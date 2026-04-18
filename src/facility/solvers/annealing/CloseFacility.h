#pragma once

#include <random>
#include <span>
#include <unordered_map>
#include <vector>

#include "State.h"
#include "helpers/Random.h"
#include "utils/Accumulators.h"

namespace facility {

struct CloseFacilityAction {
  size_t facility;
};

class CloseFacilityManager {
  std::default_random_engine random_;

  size_t choose_facility(const SolutionState& state) {
    return state.opened[rnd::index(state.opened.size(), random_)];
  }

 public:
  explicit CloseFacilityManager(const Problem& problem) {}

  CloseFacilityAction generate(const SolutionState& state) {
    return CloseFacilityAction{choose_facility(state)};
  }

  std::pair<double, double> get_gain(const SolutionState& state,
                                     CloseFacilityAction action) {
    // close current facility
    // each customer is connected to the closest opened facility
    const auto [n, d] = state.problem.shape();
    const auto& facilities = state.problem.facilities;

    std::unordered_map<size_t, double> new_demands;

    double gain = facilities[action.facility].cost;

    for (size_t i = 0; i < d; ++i) {
      if (state.solution.facility[i] != action.facility) {
        continue;
      }

      const Customer& customer = state.problem.customers[i];

      ArgMinimum<double> closest;
      for (const size_t facility : state.opened) {
        if (facility == action.facility) {
          continue;
        }

        closest.record(facility, distance_sqr(facilities[facility].position,
                                              customer.position));
      }

      const size_t new_facility = *closest.argmin();

      auto new_itr =
          new_demands.emplace(new_facility, state.demands[new_facility]).first;

      new_itr->second += customer.demand;

      gain +=
          distance(facilities[action.facility].position, customer.position) -
          distance(facilities[new_facility].position, customer.position);
    }

    double infeasibility_gain = std::max(
        state.demands[action.facility] - facilities[action.facility].capacity,
        0.);

    for (const auto [facility, new_demand] : new_demands) {
      const double old_demand = state.demands[facility];

      infeasibility_gain +=
          std::max(old_demand - facilities[facility].capacity, 0.) -
          std::max(new_demand - facilities[facility].capacity, 0.);
    }

    return {gain, infeasibility_gain};
  }

  void apply_action(SolutionState& state, CloseFacilityAction action) {
    const auto [n, d] = state.problem.shape();
    const auto& facilities = state.problem.facilities;

    bool changed_feasibility = false;

    std::erase(state.opened, action.facility);
    state.closed.push_back(action.facility);
    state.demands[action.facility] = 0;

    for (size_t i = 0; i < d; ++i) {
      if (state.solution.facility[i] != action.facility) {
        continue;
      }

      const Customer& customer = state.problem.customers[i];

      ArgMinimum<double> closest;
      for (const size_t facility : state.opened) {
        closest.record(facility, distance_sqr(facilities[facility].position,
                                              customer.position));
      }

      const size_t new_facility = *closest.argmin();

      state.solution.facility[i] = new_facility;

      const double old_demand = state.demands[new_facility];
      const double new_demand = state.demands[new_facility] + customer.demand;

      if (old_demand <= facilities[new_facility].capacity !=
          new_demand <= facilities[new_facility].capacity) {
        changed_feasibility = true;
      }

      state.demands[new_facility] += customer.demand;
    }

    if (changed_feasibility) {
      state.update_infeasible_customers();
    }
  }
};

}  // namespace facility
