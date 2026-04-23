#pragma once

#include <print>
#include <random>

#include "helpers/Time.h"
#include "setcover/CoveringSetsPack.h"
#include "setcover/Evaluator.h"
#include "setcover/Types.h"

namespace setcover {

class HillClimber3 {
  const Problem& problem;
  const timing::Deadline deadline;

  const size_t max_radius;

  CoveringSetsPack pack_;

  // Should achieve <= required_cost if possible
  std::optional<std::vector<size_t>> finish_solution(size_t required_cost) {
    std::vector<size_t> added_sets;

    auto candidates = pack_.k_max_cost_sets(10);

    for (const size_t i : candidates | std::views::values) {
      if (problem.sets[i].cost > required_cost) {
        continue;
      }

      size_t current_cost = problem.sets[i].cost;

      added_sets.push_back(i);
      pack_.include_set(i);

      while (true) {
        auto chosen_set = pack_.max_cost_set();

        if (!chosen_set.has_value()) {
          return added_sets;
        }

        if (current_cost + problem.sets[chosen_set->first].cost >
            required_cost) {
          break;
        }

        added_sets.push_back(chosen_set->first);
        pack_.include_set(chosen_set->first);
        current_cost += problem.sets[chosen_set->first].cost;
      }

      for (const size_t set : added_sets) {
        pack_.default_set(set);
      }
      added_sets.clear();
    }

    return std::nullopt;
  }

  // Returns better solution if it can be found
  std::optional<Solution> traverse_neighborhood(const Solution& center,
                                                size_t radius) {
    const size_t score = get_score(problem, center);

    pack_.reset();
    for (const size_t set : center.chosen_sets) {
      pack_.include_set(set);
    }

    // try removing radius sets
    std::vector<bool> mask(center.chosen_sets.size(), false);
    std::fill_n(mask.begin(), radius, true);

    do {
      size_t cost_decrement = 0;

      for (size_t i = 0; i < mask.size(); ++i) {
        if (mask[i]) {
          pack_.default_set(center.chosen_sets[i]);
          cost_decrement += problem.sets[center.chosen_sets[i]].cost;
        }
      }

      // restore solution in a greedy fashion
      const auto added_sets = finish_solution(cost_decrement - 1);

      if (added_sets) {
        std::vector<size_t> new_solution;
        for (size_t i = 0; i < center.chosen_sets.size(); ++i) {
          if (!mask[i]) {
            new_solution.push_back(center.chosen_sets[i]);
          }
        }

        for (size_t set : *added_sets) {
          new_solution.push_back(set);
        }

        return Solution{new_solution};
      }

      // roll back
      for (size_t i = 0; i < mask.size(); ++i) {
        if (mask[i]) {
          pack_.include_set(center.chosen_sets[i]);
        }
      }
    } while (std::ranges::prev_permutation(mask).found);

    return std::nullopt;
  }

 public:
  explicit HillClimber3(const Problem& problem, timing::Deadline deadline,
                        size_t max_radius)
      : problem(problem),
        deadline(deadline),
        max_radius(max_radius),
        pack_(problem) {}

  Solution solve(const Solution& initial_solution) {
    size_t radius = 1;

    while (!deadline.is_over()) {
      if (radius > max_radius) {
        break;
      }

      std::println("  radius = {}", radius);
      auto solution = traverse_neighborhood(initial_solution, radius);

      if (solution) {
        return *solution;
      }

      ++radius;
    }

    return initial_solution;
  }
};

}  // namespace setcover
