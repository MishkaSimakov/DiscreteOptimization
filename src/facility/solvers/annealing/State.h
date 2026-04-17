#pragma once

#include <unordered_set>
#include <vector>

#include "facility/Types.h"
#include "facility/solvers/Neighborhood.h"

namespace facility {

struct SolutionState {
  const Problem& problem;

  // customer-customer neighborhood
  std::vector<std::vector<size_t>> neighbors;

  // current solution
  Solution solution;

  // demands_[i] is the sum of i-th facility customers demands
  std::vector<double> demands;

  // indices of opened facilities
  std::vector<size_t> opened;

  // indices of closed facilities
  std::vector<size_t> closed;

  // customers that belong to infeasible facilities
  std::vector<size_t> infeasible_customers;

  SolutionState(const Problem& problem, const Solution& solution)
      : problem(problem),
        neighbors(get_neighbors(problem, 100)),
        solution(solution),
        demands(problem.facilities.size(), 0) {
    std::unordered_set<size_t> opened_set;

    for (size_t i = 0; i < problem.customers.size(); ++i) {
      demands[solution.facility[i]] += problem.customers[i].demand;
      opened_set.insert(solution.facility[i]);
    }

    opened = {opened_set.begin(), opened_set.end()};

    for (size_t i = 0; i < problem.facilities.size(); ++i) {
      if (!opened_set.contains(i)) {
        closed.push_back(i);
      }
    }

    for (size_t i = 0; i < problem.customers.size(); ++i) {
      const size_t f = solution.facility[i];

      if (demands[f] > problem.facilities[f].capacity) {
        infeasible_customers.push_back(i);
      }
    }
  }

  void update_infeasible_customers() {
    infeasible_customers.clear();

    for (size_t i = 0; i < problem.customers.size(); ++i) {
      const size_t f = solution.facility[i];

      if (demands[f] > problem.facilities[f].capacity) {
        infeasible_customers.push_back(i);
      }
    }
  }
};

}  // namespace facility
