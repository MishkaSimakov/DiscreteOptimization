#pragma once

#include <iostream>
#include <map>
#include <random>

#include "helpers/Random.h"
#include "helpers/Time.h"
#include "knapsack/Evaluator.h"
#include "knapsack/Types.h"

namespace knapsack {

class HillClimber {
  const timing::Deadline deadline;
  const size_t max_iterations_without_change;

  std::multimap<double, size_t, std::greater<>> free_items_;

  size_t iteration_ = 0;
  std::vector<size_t> taboo_;
  size_t taboo_duration_;

  std::default_random_engine random_;

  void finish_solution(const Problem& problem, Solution& solution) {
    size_t current_weight = get_weight(problem, solution);

    std::bernoulli_distribution distribution(0.1);

    for (auto itr = free_items_.begin(); itr != free_items_.end();) {
      if (distribution(random_)) {
        continue;
      }

      const size_t item = itr->second;

      if (current_weight + problem.items[item].weight > problem.max_weight) {
        ++itr;
        continue;
      }

      current_weight += problem.items[item].weight;

      solution.chosen_items.push_back(item);
      itr = free_items_.erase(itr);
    }
  }

  std::optional<size_t> remove_random_item(Solution& solution) {
    if (solution.chosen_items.empty()) {
      return std::nullopt;
    }

    size_t removed_item_index;
    size_t removed_item;

    size_t iteration = 0;

    do {
      removed_item_index = rnd::index(solution.chosen_items.size(), random_);
      removed_item = solution.chosen_items[removed_item_index];

      ++iteration;
    } while (taboo_[removed_item] != -1 &&
             taboo_[removed_item] + taboo_duration_ > iteration_ &&
             iteration < 10);

    solution.chosen_items.erase(solution.chosen_items.begin() +
                                removed_item_index);

    return removed_item;
  }

 public:
  explicit HillClimber(timing::Deadline deadline,
                       size_t max_iterations_without_change = 50)
      : deadline(deadline),
        max_iterations_without_change(max_iterations_without_change) {}

  Solution solve(const Problem& problem, const Solution& initial_solution) {
    Solution current_solution = initial_solution;
    size_t current_cost = get_score(problem, current_solution);

    size_t iterations_since_change = 0;

    taboo_duration_ =
        std::ceil(std::sqrt(initial_solution.chosen_items.size()));
    taboo_.resize(problem.items.size(), -1);

    // std::println("hill climber start, taboo duration = {}", taboo_duration_);
    // std::cout << "  " << current_cost << " " << current_solution <<
    // std::endl;

    for (size_t i = 0; i < problem.items.size(); ++i) {
      free_items_.emplace(problem.items[i].relative_cost(), i);
    }

    // TODO: do it faster, make us stronger
    for (size_t item : initial_solution.chosen_items) {
      const auto itr = std::ranges::find_if(
          free_items_,
          [item](std::pair<double, size_t> p) { return p.second == item; });
      free_items_.erase(itr);
    }

    while (true) {
      auto removed_item = remove_random_item(current_solution);

      // restore solution in a greedy fashion
      size_t prev_size = current_solution.chosen_items.size();
      finish_solution(problem, current_solution);

      if (removed_item) {
        taboo_[*removed_item] = iteration_;

        free_items_.emplace(problem.items[*removed_item].relative_cost(),
                            *removed_item);
      }

      size_t cost = get_score(problem, current_solution);

      // check that we are not in local optimum
      if (cost > current_cost) {
        iterations_since_change = 0;
        // std::print("{} -> ", current_cost, cost);
      } else {
        ++iterations_since_change;
      }

      assert(evaluate(problem, current_solution).is_valid);

      // accept new solution if it is better
      if (cost >= current_cost) {
        current_cost = cost;
        // std::cout << "  accepted: " << cost << " " << current_solution
        // << std::endl;
      } else {
        // std::cout << "  rejected: " << cost << " " << current_solution
        // << std::endl;

        // rollback
        for (size_t i = prev_size; i < current_solution.chosen_items.size();
             ++i) {
          const size_t item = current_solution.chosen_items[i];
          free_items_.emplace(problem.items[item].relative_cost(), item);
        }

        current_solution.chosen_items.resize(prev_size);

        if (removed_item) {
          current_solution.chosen_items.push_back(*removed_item);

          const auto itr = std::ranges::find_if(
              free_items_, [removed_item](std::pair<double, size_t> p) {
                return p.second == *removed_item;
              });
          free_items_.erase(itr);
        }
      }

      if (iterations_since_change > max_iterations_without_change) {
        break;
      }

      if (deadline.is_over()) {
        break;
      }

      ++iteration_;
    }

    std::println("  delta={}",
                 current_cost - get_score(problem, initial_solution));

    // std::cout << "after hill climber: " << current_solution
    // << ", score: " << current_cost << std::endl;

    return current_solution;
  }
};

}  // namespace knapsack
