#pragma once

#include <random>
#include <span>
#include <unordered_map>
#include <vector>

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
    assert(customers_.size() >= 10);
    std::iota(customers_.begin(), customers_.end(), 0);
  }

  OpenFacilityAction generate(const SolutionState& state) {
    return OpenFacilityAction{choose_facility(state)};
  }

  std::pair<double, double> get_gain(const SolutionState& state,
                                     OpenFacilityAction action) {
    // use simple greedy algorithm to grab some customers to it

    // take 10 closest customers as candidates
    auto distance_proj = [&](size_t customer) {
      return std::pair{
          distance_sqr(state.problem.customers[customer].position,
                       state.problem.facilities[action.facility].position),
          customer};
    };

    const auto nth = customers_.begin() + 10;
    std::ranges::nth_element(customers_, nth, {}, distance_proj);
    std::ranges::sort(customers_.begin(), nth, {}, distance_proj);

    double demand = 0;
    double gain = 0;

    std::unordered_map<size_t, double> updated_demands;

    for (const size_t i : std::span{customers_.begin(), nth}) {
      const Customer& customer = state.problem.customers[i];

      if (demand + customer.demand >
          state.problem.facilities[action.facility].capacity) {
        continue;
      }

      auto [itr, inserted] = updated_demands.emplace(
          state.solution.facility[i],
          state.demands[state.solution.facility[i]] - customer.demand);

      if (!inserted) {
        itr->second -= customer.demand;
      }

      demand += customer.demand;
      gain += distance(
                  state.problem.facilities[state.solution.facility[i]].position,
                  customer.position) -
              distance(state.problem.facilities[action.facility].position,
                       customer.position);
    }

    gain -= state.problem.facilities[action.facility].cost;

    double infeasibility_gain = 0;
    for (const auto [facility, new_demand] : updated_demands) {
      const double capacity = state.problem.facilities[facility].capacity;

      if (state.demands[facility] > capacity && new_demand <= capacity) {
        infeasibility_gain += state.demands[facility] - capacity;
      }

      // some facilities may accidentally be closed
      if (new_demand < 1e-10) {
        gain += state.problem.facilities[facility].cost;
      }
    }

    return {gain, infeasibility_gain};
  }

  void apply_action(SolutionState& state, OpenFacilityAction action) {
    // use simple greedy algorithm to grab some customers to it

    // take 10 closest customers as candidates
    auto distance_proj = [&](size_t customer) {
      return std::pair{
          distance_sqr(state.problem.customers[customer].position,
                       state.problem.facilities[action.facility].position),
          customer};
    };

    const auto nth = customers_.begin() + 10;
    std::ranges::nth_element(customers_, nth, {}, distance_proj);
    std::ranges::sort(customers_.begin(), nth, {}, distance_proj);

    bool changed_feasibility = false;

    std::unordered_set<size_t> closed_facilities;

    for (const size_t i : std::span{customers_.begin(), nth}) {
      const Customer& customer = state.problem.customers[i];

      if (state.demands[action.facility] + customer.demand >
          state.problem.facilities[action.facility].capacity) {
        continue;
      }

      const size_t old_facility = state.solution.facility[i];
      const double capacity = state.problem.facilities[old_facility].capacity;

      if (state.demands[old_facility] > capacity &&
          state.demands[old_facility] - customer.demand <= capacity) {
        changed_feasibility = true;
      }

      state.demands[action.facility] += customer.demand;
      state.demands[old_facility] -= customer.demand;
      state.solution.facility[i] = action.facility;

      if (state.demands[old_facility] < 1e-10) {
        closed_facilities.insert(old_facility);
      }
    }

    std::erase(state.closed, action.facility);
    state.opened.push_back(action.facility);

    std::erase_if(state.opened,
                  [&](size_t i) { return closed_facilities.contains(i); });
    for (const size_t i : closed_facilities) {
      state.closed.push_back(i);
    }

    if (changed_feasibility) {
      state.update_infeasible_customers();
    }
  }
};

}  // namespace facility
