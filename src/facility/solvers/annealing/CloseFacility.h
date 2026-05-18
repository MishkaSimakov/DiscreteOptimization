#pragma once

#include <random>
#include <span>
#include <unordered_map>
#include <vector>

#include "helpers/Random.h"
#include "utils/Accumulators.h"

namespace facility {

struct CloseFacilityAction {
  size_t facility;
};

class CloseFacilityManager {
 public:
  using Action = CloseFacilityAction;

 private:
  std::default_random_engine random_;

  const ProblemState& state_;

  size_t choose_facility(const SolutionState& state) {
    return state.opened[rnd::index(state.opened.size(), random_)];
  }

 public:
  explicit CloseFacilityManager(const ProblemState& problem)
      : state_(problem) {}

  std::optional<CloseFacilityAction> generate(
      const SolutionState& state, annealing::SolverStateDTO solver) {
    if (state.opened.size() <= 1) {
      return std::nullopt;
    }

    return CloseFacilityAction{choose_facility(state)};
  }

  annealing::ActionGain get_gain(const SolutionState& solution,
                                 CloseFacilityAction action) {
    // close current facility
    // each customer is connected to the closest opened facility
    const auto [n, d] = state_.problem.shape();
    const auto& facilities = state_.problem.facilities;

    std::unordered_map<size_t, double> new_capacities;

    double gain = facilities[action.facility].cost;

    for (size_t i = 0; i < d; ++i) {
      if (solution.solution.facility[i] != action.facility) {
        continue;
      }

      const Customer& customer = state_.problem.customers[i];

      ArgMinimum<double> closest;
      for (const size_t facility : solution.opened) {
        if (facility == action.facility) {
          continue;
        }

        closest.record(facility, distance_sqr(facilities[facility].position,
                                              customer.position));
      }

      const size_t new_facility = closest->index;

      auto new_itr =
          new_capacities.emplace(new_facility, solution.capacity[new_facility])
              .first;

      new_itr->second -= customer.demand;

      gain +=
          distance(facilities[action.facility].position, customer.position) -
          distance(facilities[new_facility].position, customer.position);
    }

    double infeasibility_gain =
        std::max(-solution.capacity[action.facility], 0.);

    for (const auto [facility, new_capacity] : new_capacities) {
      infeasibility_gain += std::max(-solution.capacity[facility], 0.) -
                            std::max(-new_capacity, 0.);
    }

    return {gain, infeasibility_gain};
  }

  void apply_action(SolutionState& solution, CloseFacilityAction action,
                    annealing::SolverStateDTO solver) {
    const auto [n, d] = state_.problem.shape();
    const auto& facilities = state_.problem.facilities;

    bool changed_feasibility = false;

    std::erase(solution.opened, action.facility);
    solution.closed.push_back(action.facility);
    solution.capacity[action.facility] =
        state_.problem.facilities[action.facility].capacity;

    for (size_t i = 0; i < d; ++i) {
      if (solution.solution.facility[i] != action.facility) {
        continue;
      }

      const Customer& customer = state_.problem.customers[i];

      ArgMinimum<double> closest;
      for (const size_t facility : solution.opened) {
        closest.record(facility, distance_sqr(facilities[facility].position,
                                              customer.position));
      }

      const size_t new_facility = closest->index;

      solution.solution.facility[i] = new_facility;

      const double new_capacity =
          solution.capacity[new_facility] - customer.demand;
      if (solution.capacity[new_facility] <= -1e-10 != new_capacity <= -1e-10) {
        changed_feasibility = true;
      }

      solution.capacity[new_facility] -= customer.demand;
    }

    if (changed_feasibility) {
      solution.update_infeasible_customers();
    }
  }
};

}  // namespace facility
