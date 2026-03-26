#pragma once

#include <print>
#include <random>

#include "helpers/Time.h"
#include "setcover/CoveringSetsPack.h"
#include "setcover/Types.h"

namespace setcover {

class HillClimber {
  const timing::Deadline deadline;
  const size_t max_iterations_without_change;

  static void finish_solution(const Problem& problem, Solution& solution) {
    CoveringSetsPack pack(problem);

    for (const size_t set_index : solution.chosen_sets) {
      pack.cover_set(set_index);
    }

    // Достраиваем жадным способом
    while (true) {
      auto chosen_set = pack.max_cost_set();

      if (!chosen_set.has_value()) {
        break;
      }

      solution.chosen_sets.push_back(chosen_set->first);
      pack.cover_set(chosen_set->first);
    }
  }

 public:
  explicit HillClimber(timing::Deadline deadline,
                       size_t max_iterations_without_change = 50)
      : deadline(deadline),
        max_iterations_without_change(max_iterations_without_change) {}

  Solution solve(const Problem& problem, const Solution& initial_solution) {
    std::default_random_engine engine;

    Solution current_solution = initial_solution;
    size_t current_cost = get_score(problem, current_solution);

    size_t iterations_since_change = 0;

    while (true) {
      std::uniform_int_distribution<size_t> random_element(
          0, current_solution.chosen_sets.size() - 1);

      size_t removed_set = random_element(engine);

      // remove one set
      Solution new_solution = current_solution;
      new_solution.chosen_sets.erase(new_solution.chosen_sets.begin() +
                                     removed_set);

      // restore solution in a greedy fashion
      finish_solution(problem, new_solution);

      size_t cost = get_score(problem, new_solution);

      // check that we are not in local optimum
      if (cost < current_cost) {
        iterations_since_change = 0;
      } else {
        ++iterations_since_change;
      }

      if (iterations_since_change > max_iterations_without_change) {
        break;
      }

      // accept new solution if it is better
      if (cost <= current_cost) {
        current_solution = std::move(new_solution);
        current_cost = cost;

        std::println("  {} -> {}", current_cost, cost);
      }

      if (deadline.is_over()) {
        break;
      }
    }

    return current_solution;
  }
};

}  // namespace setcover
