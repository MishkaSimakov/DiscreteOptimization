#pragma once

#include <print>
#include <random>

#include "helpers/Time.h"
#include "setcover/CoveringSetsPack.h"
#include "setcover/Evaluator.h"
#include "setcover/Types.h"

namespace setcover {

class HillClimber2 {
  const timing::Deadline deadline;

  static void finish_solution(const Problem& problem, Solution& solution,
                              CoveringSetsPack& pack) {
    pack.reset();

    for (const size_t set_index : solution.chosen_sets) {
      pack.include_set(set_index);
    }

    // Достраиваем жадным способом
    while (true) {
      auto chosen_set = pack.max_cost_set();

      if (!chosen_set.has_value()) {
        break;
      }

      solution.chosen_sets.push_back(chosen_set->first);
      pack.include_set(chosen_set->first);
    }
  }

  size_t try_to_finish(const Problem& problem, Solution& solution,
                       CoveringSetsPack& pack, size_t current_best) {
    pack.reset();

    for (const size_t set_index : solution.chosen_sets) {
      pack.include_set(set_index);
    }

    size_t size = solution.chosen_sets.size();
    auto first_candidates = pack.get_default_sets(0);

    for (const size_t candidate : first_candidates) {
      pack.reset();

      for (const size_t set_index : solution.chosen_sets) {
        pack.include_set(set_index);
      }

      pack.include_set(candidate);
      solution.chosen_sets.push_back(candidate);

      while (true) {
        auto chosen_set = pack.max_cost_set();

        if (!chosen_set.has_value()) {
          break;
        }

        solution.chosen_sets.push_back(chosen_set->first);
        pack.include_set(chosen_set->first);
      }

      const size_t new_cost = get_score(problem, solution);

      // accept new solution if it is better
      if (new_cost < current_best) {
        // std::println("  {} -> {}", current_cost, new_cost);
        return new_cost;
      } else {
        // rollback changes
        solution.chosen_sets.resize(size);
      }

      if (deadline.is_over()) {
        break;
      }
    }

    return current_best;
  }

 public:
  explicit HillClimber2(timing::Deadline deadline) : deadline(deadline) {}

  Solution solve(const Problem& problem, const Solution& initial_solution) {
    std::default_random_engine random;

    Solution current_solution = initial_solution;
    size_t current_best = get_score(problem, current_solution);

    CoveringSetsPack pack(problem);

    while (true) {
      const size_t size = current_solution.chosen_sets.size();

      std::vector<size_t> order(size);
      std::iota(order.begin(), order.end(), 0);
      std::ranges::shuffle(order, random);

      bool changed = false;

      for (const size_t removed_set_index : order) {
        // remove one set
        const size_t removed_set =
            current_solution.chosen_sets[removed_set_index];
        current_solution.chosen_sets.erase(
            current_solution.chosen_sets.begin() + removed_set_index);

        const size_t new_cost =
            try_to_finish(problem, current_solution, pack, current_best);

        if (new_cost < current_best) {
          current_best = new_cost;
          changed = true;
          break;
        }

        current_solution.chosen_sets.insert(
            current_solution.chosen_sets.begin() + removed_set_index,
            removed_set);

        if (deadline.is_over()) {
          break;
        }
      }

      if (!changed) {
        break;
      }
    }

    return current_solution;
  }
};

}  // namespace setcover
