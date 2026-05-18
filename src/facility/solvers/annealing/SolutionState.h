#pragma once

#include <unordered_set>
#include <vector>

#include "facility/Types.h"
#include "facility/solvers/Neighborhood.h"

namespace facility {

struct SolutionState {
  // current solution
  Solution solution;

  // capacity[i] = C - D, where
  // C is the capacity of i-th facility,
  // D is the sum of i-th facility customers demands.
  std::vector<double> capacity;

  // indices of opened facilities
  std::vector<size_t> opened;

  // indices of closed facilities
  std::vector<size_t> closed;

  // customers that belong to infeasible facilities
  std::vector<size_t> infeasible_customers;

  SolutionState(const Problem& problem, const Solution& solution)
      : solution(solution), capacity(problem.facilities.size()) {
    for (size_t i = 0; i < problem.facilities.size(); ++i) {
      capacity[i] = problem.facilities[i].capacity;
    }

    std::unordered_set<size_t> opened_set;

    for (size_t i = 0; i < problem.customers.size(); ++i) {
      capacity[solution.facility[i]] -= problem.customers[i].demand;
      opened_set.insert(solution.facility[i]);
    }

    opened = {opened_set.begin(), opened_set.end()};

    for (size_t i = 0; i < problem.facilities.size(); ++i) {
      if (!opened_set.contains(i)) {
        closed.push_back(i);
      }
    }

    update_infeasible_customers();
  }

  Solution get_solution() const { return solution; }

  void update_infeasible_customers() {
    infeasible_customers.clear();

    for (size_t i = 0; i < solution.facility.size(); ++i) {
      if (capacity[solution.facility[i]] < -1e-10) {
        infeasible_customers.push_back(i);
      }
    }
  }

  // verify that capacities are updated correctly
  bool check_capacity(const Problem& problem) const {
    std::vector<double> expected(problem.facilities.size());

    for (size_t i = 0; i < problem.facilities.size(); ++i) {
      expected[i] = problem.facilities[i].capacity;
    }

    for (size_t i = 0; i < problem.customers.size(); ++i) {
      expected[solution.facility[i]] -= problem.customers[i].demand;
    }

    return expected == capacity;
  }
};

}  // namespace facility
