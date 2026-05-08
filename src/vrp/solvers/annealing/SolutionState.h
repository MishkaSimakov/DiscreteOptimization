#pragma once

#include <vector>

#include "ProblemState.h"

namespace vrp {

struct SolutionState {
  const size_t customers_count;

  struct CustomerNode {
    size_t next;
    size_t prev;
    size_t vehicle;
  };
  std::vector<CustomerNode> customers;

  std::vector<double> vehicle_demands;

  SolutionState(const ProblemState& state, const Solution& initial_solution)
      : customers_count(state.problem.customers.size()),
        customers(state.problem.customers.size() +
                  state.problem.vehicles_count),
        vehicle_demands(state.problem.vehicles_count) {
    for (size_t i = 0; i < state.problem.vehicles_count; ++i) {
      const auto& route = initial_solution.routes[i];

      // sentinel node
      const size_t sentinel_index = customers_count + i;
      customers[customers_count + i] = CustomerNode{
          .next = route.empty() ? sentinel_index : route.front(),
          .prev = route.empty() ? sentinel_index : route.back(),
          .vehicle = i,
      };

      for (size_t j = 0; j < route.size(); ++j) {
        customers[route[j]] = CustomerNode{
            .next = j + 1 == route.size() ? sentinel_index : route[j + 1],
            .prev = j == 0 ? sentinel_index : route[j - 1],
            .vehicle = i,
        };
        vehicle_demands[i] += state.problem.customers[route[j]].demand;
      }
    }
  }

  Solution get_solution() const {
    const size_t vehicles_count = vehicle_demands.size();

    std::vector<std::vector<size_t>> result(vehicles_count);

    for (size_t i = 0; i < vehicles_count; ++i) {
      size_t current = customers[customers_count + i].next;

      while (current != customers_count + i) {
        assert(customers[current].vehicle == i);
        result[i].push_back(current);

        current = customers[current].next;
      }
    }

    return Solution{std::move(result)};
  }
};

}  // namespace vrp
