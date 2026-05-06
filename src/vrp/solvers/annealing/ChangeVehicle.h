#pragma once

#include <random>

#include "ProblemState.h"
#include "SolutionState.h"
#include "common/annealing/ActionGain.h"
#include "helpers/Random.h"

namespace vrp {

struct ChangeVehicleAction {
  size_t customer;
  size_t new_vehicle;
  size_t injection_position;
};

class ChangeVehicleManager {
 public:
  using Action = ChangeVehicleAction;

 private:
  const ProblemState& state_;
  std::default_random_engine random_;

 public:
  explicit ChangeVehicleManager(const ProblemState& problem)
      : state_(problem) {}

  std::optional<ChangeVehicleAction> generate(const SolutionState& solution) {
    const size_t n = state_.problem.customers.size();

    const size_t customer = rnd::index(n, random_);

    size_t new_vehicle = rnd::index(state_.problem.vehicles_count - 1, random_);
    if (new_vehicle >= solution.customers[customer].vehicle) {
      ++new_vehicle;
    }

    // find injection_point using greedy algorithm
    ArgMaximum<double> max_gain;

    size_t current = n + new_vehicle;

    do {
      assert(solution.customers[current].vehicle == new_vehicle);

      const double gain =
          state_.get_distance(current, solution.customers[current].next) -
          state_.get_distance(current, customer) -
          state_.get_distance(customer, solution.customers[current].next);

      max_gain.record(current, gain);

      current = solution.customers[current].next;
    } while (current != n + new_vehicle);

    assert(new_vehicle != solution.customers[customer].vehicle);
    assert(solution.customers[max_gain->index].vehicle == new_vehicle);

    return ChangeVehicleAction{
        .customer = customer,
        .new_vehicle = new_vehicle,
        .injection_position = max_gain->index,
    };
  }

  annealing::ActionGain get_gain(const SolutionState& solution,
                                 ChangeVehicleAction action) {
    const auto old_node = solution.customers[action.customer];
    const auto new_node = SolutionState::CustomerNode{
        .next = solution.customers[action.injection_position].next,
        .prev = action.injection_position,
        .vehicle = action.new_vehicle,
    };

    const double score_gain =
        state_.get_distance(old_node.prev, action.customer) +
        state_.get_distance(action.customer, old_node.next) +
        state_.get_distance(new_node.prev, new_node.next) -
        state_.get_distance(old_node.prev, old_node.next) -
        state_.get_distance(new_node.prev, action.customer) -
        state_.get_distance(action.customer, new_node.next);

    const double customer_demand =
        state_.problem.customers[action.customer].demand;
    const double vehicle_capacity = state_.problem.vehicle_capacity;

    const double old_demand = solution.vehicle_demands[old_node.vehicle];
    const double new_demand = solution.vehicle_demands[new_node.vehicle];

    const double infeasibility_gain =
        std::max(0., old_demand - vehicle_capacity) +
        std::max(0., new_demand - vehicle_capacity) -
        std::max(0., old_demand - customer_demand - vehicle_capacity) -
        std::max(0., new_demand + customer_demand - vehicle_capacity);

    return annealing::ActionGain{
        .score = score_gain,
        .infeasibility = infeasibility_gain,
    };
  }

  void apply_action(SolutionState& solution, ChangeVehicleAction action) {
    const auto old_node = solution.customers[action.customer];
    const auto new_node = SolutionState::CustomerNode{
        .next = solution.customers[action.injection_position].next,
        .prev = action.injection_position,
        .vehicle = action.new_vehicle,
    };

    const double customer_demand =
        state_.problem.customers[action.customer].demand;

    solution.vehicle_demands[old_node.vehicle] -= customer_demand;
    solution.vehicle_demands[new_node.vehicle] += customer_demand;

    solution.customers[action.customer] = new_node;

    solution.customers[old_node.prev].next = old_node.next;
    solution.customers[old_node.next].prev = old_node.prev;

    solution.customers[new_node.prev].next = action.customer;
    solution.customers[new_node.next].prev = action.customer;
  }
};

}  // namespace vrp
