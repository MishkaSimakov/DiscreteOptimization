#pragma once

#include <optional>
#include <vector>

#include "facility/Types.h"
#include "facility/solvers/Neighborhood.h"

namespace facility {

class TwoOptImprover {
  constexpr static size_t max_iterations = 100;
  constexpr static size_t neighborhood_size = 10;

  const Problem& problem;

  // for each customer stores his neighbors, that is k closest customers
  const std::vector<std::vector<size_t>> neighbors;

 public:
  explicit TwoOptImprover(const Problem& problem)
      : problem(problem),
        neighbors(get_neighbors(problem, neighborhood_size)) {}

  std::vector<size_t> improve(std::vector<size_t> solution,
                              std::vector<double> demands) {
    const auto [n, d] = problem.shape();

    const auto& customers = problem.customers;
    const auto& facilities = problem.facilities;

    bool changed;

    size_t iteration = 0;

    do {
      ++iteration;

      changed = false;

      for (size_t i = 0; i < d; ++i) {
        // try to swap with someone
        for (size_t j : neighbors[i]) {
          if (solution[i] == solution[j]) {
            continue;
          }

          // try to change j-th customer facility to assigned_facility
          if (demands[solution[i]] - customers[i].demand + customers[j].demand >
              facilities[solution[i]].capacity) {
            continue;
          }

          if (demands[solution[j]] + customers[i].demand - customers[j].demand >
              facilities[solution[j]].capacity) {
            continue;
          }

          const double gain =
              distance(customers[i].position,
                       facilities[solution[i]].position) +
              distance(customers[j].position,
                       facilities[solution[j]].position) -
              distance(customers[i].position,
                       facilities[solution[j]].position) -
              distance(customers[j].position, facilities[solution[i]].position);

          if (gain < 1e-10) {
            continue;
          }

          demands[solution[i]] += customers[j].demand - customers[i].demand;
          demands[solution[j]] += customers[i].demand - customers[j].demand;

          std::swap(solution[i], solution[j]);

          changed = true;
        }
      }
    } while (changed && iteration < max_iterations);

    return solution;
  }
};

}  // namespace facility
