#pragma once

#include <random>
#include <optional>

#include "ProblemState.h"
#include "SolutionState.h"
#include "helpers/Random.h"

namespace facility {

struct ChangeCustomerFacilityAction {
  size_t customer;
  size_t facility;
};

class ChangeCustomerFacilityManager {
 public:
  using Action = ChangeCustomerFacilityAction;

 private:
  const ProblemState& state_;

  std::default_random_engine random_;

  std::vector<size_t> buffer_;

  // for each customer stores last iteration when his facility was changed
  std::vector<size_t> last_change_;

  static constexpr size_t taboo_duration = 0;

  size_t choose_customer_random(const SolutionState& solution,
                                size_t changes_count) {
    return rnd::index(state_.problem.customers.size(), random_);
  }

  size_t choose_customer(const SolutionState& solution, size_t changes_count) {
    size_t chosen_customer;
    size_t choose_iteration = 0;

    do {
      if (rnd::bernoulli(0.5, random_) &&
          !solution.infeasible_customers.empty()) {
        chosen_customer = solution.infeasible_customers[rnd::index(
            solution.infeasible_customers.size(), random_)];
      } else {
        chosen_customer = rnd::index(state_.problem.customers.size(), random_);
      }

      ++choose_iteration;
    } while (last_change_[chosen_customer] != 0 &&
             changes_count - last_change_[chosen_customer] < taboo_duration &&
             choose_iteration < 10);

    return chosen_customer;
  }

  size_t choose_facility_random(const SolutionState& solution,
                                size_t customer) {
    assert(!solution.opened.empty());

    size_t current_index;
    for (size_t i = 0; i < solution.opened.size(); ++i) {
      if (solution.opened[i] == solution.solution.facility[customer]) {
        current_index = i;
        break;
      }
    }

    size_t index = rnd::index(solution.opened.size() - 1, random_);
    if (index >= current_index) {
      ++index;
    }

    return solution.opened[index];
  }

  // choose facility among those that are common among neighbors
  size_t choose_facility_neighbors(const SolutionState& solution,
                                   size_t customer) {
    buffer_.clear();

    for (size_t i = 0; i < state_.neighbors[customer].size(); ++i) {
      const size_t f =
          solution.solution.facility[state_.neighbors[customer][i]];

      if (f != solution.solution.facility[customer]) {
        buffer_.push_back(f);
      }
    }

    if (!buffer_.empty()) {
      return buffer_[rnd::index(buffer_.size(), random_)];
    }

    // just choose the closest one
    ArgMinimum<double, std::less<>> closest;

    for (size_t facility : solution.opened) {
      if (facility != solution.solution.facility[customer]) {
        closest.record(
            facility,
            distance_sqr(state_.problem.facilities[facility].position,
                         state_.problem.customers[customer].position));
      }
    }

    return closest->index;
  }

 public:
  explicit ChangeCustomerFacilityManager(const ProblemState& state)
      : state_(state), last_change_(state.problem.customers.size(), 0) {}

  std::optional<ChangeCustomerFacilityAction> generate(
      const SolutionState& solution, annealing::SolverStateDTO solver) {
    if (solution.opened.size() == 1) {
      return std::nullopt;
    }

    const size_t customer =
        choose_customer_random(solution, solver.changes_count);
    const size_t facility = choose_facility_random(solution, customer);

    assert(facility != solution.solution.facility[customer]);

    return ChangeCustomerFacilityAction{customer, facility};
  }

  annealing::ActionGain get_gain(const SolutionState& solution,
                                 ChangeCustomerFacilityAction action) {
    const size_t f0 = solution.solution.facility[action.customer];
    const size_t f1 = action.facility;

    const Customer& customer = state_.problem.customers[action.customer];

    // recalculate capacities (f0 changed to f1)
    const double new_capacity_f0 = solution.capacity[f0] + customer.demand;
    const double new_capacity_f1 = solution.capacity[f1] - customer.demand;

    double gain =
        distance(customer.position, state_.problem.facilities[f0].position) -
        distance(customer.position, state_.problem.facilities[f1].position);

    // f0 may accidentally close
    if (state_.problem.facilities[f0].capacity - new_capacity_f0 < 1e-10) {
      gain += state_.problem.facilities[f0].cost;
    }

    const double infeasibility_gain =
        std::max(-solution.capacity[f0], 0.) - std::max(-new_capacity_f0, 0.) +
        std::max(-solution.capacity[f1], 0.) - std::max(-new_capacity_f1, 0.);

    return {gain, infeasibility_gain};
  }

  void apply_action(SolutionState& state, ChangeCustomerFacilityAction action,
                    annealing::SolverStateDTO solver) {
    last_change_[action.customer] = solver.changes_count;

    const size_t f0 = state.solution.facility[action.customer];
    const size_t f1 = action.facility;

    const double old_capacity_f0 = state.capacity[f0];
    const double old_capacity_f1 = state.capacity[f1];

    state.solution.facility[action.customer] = f1;
    state.capacity[f0] += state_.problem.customers[action.customer].demand;
    state.capacity[f1] -= state_.problem.customers[action.customer].demand;

    if (state_.problem.facilities[f0].capacity - state.capacity[f0] < 1e-10) {
      state.closed.push_back(f0);
      std::erase(state.opened, f0);
    }

    if (old_capacity_f0 < -1e-10 != state.capacity[f0] < -1e-10 ||
        old_capacity_f1 < -1e-10 != state.capacity[f1] < -1e-10) {
      state.update_infeasible_customers();
    }
  }
};

}  // namespace facility
