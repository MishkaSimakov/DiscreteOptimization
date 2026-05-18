#pragma once

#include <random>
#include <span>
#include <unordered_map>
#include <vector>

#include "ProblemState.h"
#include "SolutionState.h"
#include "common/annealing/ActionGain.h"
#include "common/annealing/SolverState.h"
#include "helpers/Random.h"

namespace facility {

struct OpenFacilityAction {
  // index of newly opened facility
  size_t facility;

  // indices of customers that go into that facility
  std::vector<size_t> customers;
};

class OpenFacilityManager {
 public:
  using Action = OpenFacilityAction;

 private:
  const ProblemState& state_;

  std::default_random_engine random_;
  std::vector<size_t> customers_;

  const std::vector<std::vector<size_t>> facility_customer_neighborhood_;

  size_t choose_facility(const SolutionState& state) {
    return state.closed[rnd::index(state.closed.size(), random_)];
  }

 public:
  explicit OpenFacilityManager(const ProblemState& problem)
      : state_(problem),
        customers_(problem.problem.customers.size()),
        facility_customer_neighborhood_(
            get_facility_customer_neighborhood(problem.problem, 20)) {
    assert(customers_.size() >= 10);
    std::iota(customers_.begin(), customers_.end(), 0);
  }

  std::optional<OpenFacilityAction> generate(const SolutionState& state,
                                             annealing::SolverStateDTO solver) {
    if (state.closed.empty()) {
      return std::nullopt;
    }

    const size_t facility_index = choose_facility(state);
    const Facility facility = state_.problem.facilities[facility_index];

    std::vector<size_t> customers;

    // use simple greedy algorithm to grab some customers for new facility

    auto distance_proj = [&](size_t customer) {
      return distance_sqr(state_.problem.customers[customer].position,
                          facility.position);
    };

    // std::ranges::sort(customers_, {}, distance_proj);

    double demand = 0;

    for (const size_t i : facility_customer_neighborhood_[facility_index]) {
      const Customer& customer = state_.problem.customers[i];

      if (demand + customer.demand > facility.capacity) {
        continue;
      }

      demand += customer.demand;
      customers.push_back(i);
    }

    return OpenFacilityAction{
        .facility = facility_index,
        .customers = std::move(customers),
    };
  }

  annealing::ActionGain get_gain(const SolutionState& solution,
                                 OpenFacilityAction action) {
    const auto& facilities = state_.problem.facilities;

    double score_gain = 0;

    std::unordered_map<size_t, double> updated_capacity;

    for (const size_t i : action.customers) {
      const size_t old_facility = solution.solution.facility[i];
      const Customer& customer = state_.problem.customers[i];

      auto [itr, inserted] = updated_capacity.emplace(
          old_facility, solution.capacity[old_facility]);

      itr->second += customer.demand;

      score_gain +=
          distance(facilities[old_facility].position, customer.position) -
          distance(facilities[action.facility].position, customer.position);
    }

    score_gain -= facilities[action.facility].cost;

    double infeasibility_gain = 0;
    for (const auto [facility, new_capacity] : updated_capacity) {
      infeasibility_gain += std::max(-solution.capacity[facility], 0.) -
                            std::max(-new_capacity, 0.);

      // some facilities may accidentally be closed
      if (facilities[facility].capacity - new_capacity < 1e-10) {
        score_gain += state_.problem.facilities[facility].cost;
      }
    }

    return {score_gain, infeasibility_gain};
  }

  void apply_action(SolutionState& solution, OpenFacilityAction action,
                    annealing::SolverStateDTO solver) {
    bool changed_feasibility = false;

    std::unordered_set<size_t> closed_facilities;

    for (const size_t i : action.customers) {
      const Customer& customer = state_.problem.customers[i];
      const size_t old_facility = solution.solution.facility[i];

      if (solution.capacity[old_facility] < -1e-10 &&
          solution.capacity[old_facility] + customer.demand >= -1e-10) {
        changed_feasibility = true;
      }

      solution.capacity[action.facility] -= customer.demand;
      solution.capacity[old_facility] += customer.demand;
      solution.solution.facility[i] = action.facility;

      if (state_.problem.facilities[old_facility].capacity -
              solution.capacity[old_facility] <
          1e-10) {
        closed_facilities.insert(old_facility);
      }
    }

    std::erase(solution.closed, action.facility);
    solution.opened.push_back(action.facility);

    std::erase_if(solution.opened,
                  [&](size_t i) { return closed_facilities.contains(i); });
    for (const size_t i : closed_facilities) {
      solution.closed.push_back(i);
    }

    if (changed_feasibility) {
      solution.update_infeasible_customers();
    }
  }
};

}  // namespace facility
