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

  std::vector<size_t> buffer_;

  // for each customer stores last iteration when his facility was changed
  std::vector<size_t> last_change_;

  static constexpr size_t taboo_duration = 500;

  size_t choose_customer(const SolutionState& state) {
    size_t chosen_customer;
    size_t choose_iteration = 0;

    do {
      if (rnd::bernoulli(0.5, random_) && !state.infeasible_customers.empty()) {
        chosen_customer = state.infeasible_customers[rnd::index(
            state.infeasible_customers.size(), random_)];
      } else {
        chosen_customer = rnd::index(state.problem.customers.size(), random_);
      }

      ++choose_iteration;
    } while (last_change_[chosen_customer] != 0 &&
             state.changes_count - last_change_[chosen_customer] <
                 taboo_duration &&
             choose_iteration < 10);

    return chosen_customer;
  }

  // choose facility among top 10 closest opened
  size_t choose_facility_closest(const SolutionState& state, size_t customer) {
    auto opened = state.opened;

    // take 10 closest opened facilities as candidates
    auto distance_proj = [&](size_t facility) {
      if (facility == state.solution.facility[customer]) {
        return 1e10;
      }

      return distance_sqr(state.problem.customers[customer].position,
                          state.problem.facilities[facility].position);
    };

    const auto nth = opened.size() < 10 ? opened.end() : opened.begin() + 10;
    std::ranges::nth_element(opened, nth, {}, distance_proj);
    std::ranges::sort(opened.begin(), nth, {}, distance_proj);

    for (const size_t facility : std::span{opened.begin(), nth}) {
      if (facility == state.solution.facility[customer]) {
        break;
      }

      if (rnd::bernoulli(0.4, random_)) {
        return facility;
      }
    }

    return opened[0];
  }

  // choose facility among those that are common among neighbors
  size_t choose_facility_neighbors(const SolutionState& state,
                                   size_t customer) {
    buffer_.clear();

    for (size_t i = 0; i < state.neighbors[customer].size(); ++i) {
      const size_t f = state.solution.facility[state.neighbors[customer][i]];

      if (f != state.solution.facility[customer]) {
        buffer_.push_back(f);
      }
    }

    if (!buffer_.empty()) {
      return buffer_[rnd::index(buffer_.size(), random_)];
    }

    // just choose the closest one
    ArgMinimum<double, std::less<>> closest;

    for (size_t facility : state.opened) {
      if (facility != state.solution.facility[customer]) {
        closest.record(
            facility, distance_sqr(state.problem.facilities[facility].position,
                                   state.problem.customers[customer].position));
      }
    }

    return *closest.argmin();
  }

 public:
  explicit ChangeCustomerFacilityManager(const Problem& problem)
      : last_change_(problem.customers.size(), 0) {}

  ChangeCustomerFacilityAction generate(const SolutionState& state) {
    const size_t customer = choose_customer(state);
    const size_t facility = choose_facility_neighbors(state, customer);

    return ChangeCustomerFacilityAction{customer, facility};
  }

  std::pair<double, double> get_gain(const SolutionState& state,
                                     ChangeCustomerFacilityAction action) {
    const size_t f0 = state.solution.facility[action.customer];
    const size_t f1 = action.facility;

    const Customer& customer = state.problem.customers[action.customer];

    // recalculate demands (f0 changed to f1)
    const double new_demand_f0 = state.demands[f0] - customer.demand;
    const double new_demand_f1 = state.demands[f1] + customer.demand;

    double gain =
        distance(customer.position, state.problem.facilities[f0].position) -
        distance(customer.position, state.problem.facilities[f1].position);

    const double capacity_f0 = state.problem.facilities[f0].capacity;
    const double capacity_f1 = state.problem.facilities[f1].capacity;

    // f0 may accidentally close
    if (new_demand_f0 < 1e-10) {
      gain += state.problem.facilities[f0].cost;
    }

    const double infeasibility_gain =
        std::max(state.demands[f0] - capacity_f0, 0.) -
        std::max(new_demand_f0 - capacity_f0, 0.) +
        std::max(state.demands[f1] - capacity_f1, 0.) -
        std::max(new_demand_f1 - capacity_f1, 0.);

    return {gain, infeasibility_gain};
  }

  void apply_action(SolutionState& state, ChangeCustomerFacilityAction action) {
    last_change_[action.customer] = state.changes_count;

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

    if (new_demand_f0 < 1e-10) {
      state.closed.push_back(f0);
      std::erase(state.opened, f0);
    }

    if (old_demand_f0 <= state.problem.facilities[f0].capacity !=
            new_demand_f0 <= state.problem.facilities[f0].capacity ||
        old_demand_f1 <= state.problem.facilities[f1].capacity !=
            new_demand_f1 <= state.problem.facilities[f1].capacity) {
      state.update_infeasible_customers();
    }
  }
};

}  // namespace facility
