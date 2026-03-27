#pragma once

#include <iostream>
#include <map>
#include <random>

#include "helpers/Random.h"
#include "helpers/Time.h"
#include "knapsack/Evaluator.h"
#include "knapsack/Types.h"

namespace knapsack {

struct SimulatedAnnealingConfig {
  double relative_start_temperature = 1e-5;
  double relative_end_temperature = 1e-10;

  double alpha = 0.99;
  size_t iterations_per_temperature = 10;
};

class SimulatedAnnealing {
  const SimulatedAnnealingConfig config;

  const timing::Deadline deadline;

  std::multimap<double, size_t, std::greater<>> free_items_;

  std::default_random_engine random_;

  void finish_solution(const Problem& problem, Solution& solution) {
    size_t current_weight = get_weight(problem, solution);

    std::bernoulli_distribution distribution(0.01);

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

  std::optional<size_t> remove_random_item(Solution& solution,
                                           const std::vector<size_t>& taboo,
                                           const size_t taboo_duration,
                                           const size_t current_iteration) {
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
    } while (taboo[removed_item] != -1 &&
             taboo[removed_item] + taboo_duration > current_iteration &&
             iteration < 10);

    solution.chosen_items.erase(solution.chosen_items.begin() +
                                removed_item_index);

    return removed_item;
  }

 public:
  explicit SimulatedAnnealing(timing::Deadline deadline,
                              SimulatedAnnealingConfig config)
      : config(config), deadline(deadline) {}

  Solution solve(const Problem& problem, const Solution& initial_solution) {
    std::uniform_real_distribution<double> random_accept(0, 1);

    Solution current_solution = initial_solution;
    size_t current_cost = get_score(problem, current_solution);

    Solution best_solution = current_solution;
    size_t best_cost = current_cost;

    size_t iteration = 0;

    const size_t taboo_duration =
        std::ceil(std::sqrt(initial_solution.chosen_items.size()));
    std::vector<size_t> taboo(problem.items.size(), -1);

    for (size_t i = 0; i < problem.items.size(); ++i) {
      free_items_.emplace(problem.items[i].relative_cost(), i);
    }

    for (const size_t item : initial_solution.chosen_items) {
      const auto itr = std::ranges::find_if(
          free_items_,
          [item](std::pair<double, size_t> p) { return p.second == item; });
      free_items_.erase(itr);
    }

    double temperature = current_cost * config.relative_start_temperature;
    const double end_temperature =
        current_cost * config.relative_end_temperature;

    while (temperature > end_temperature) {
      auto removed_item = remove_random_item(current_solution, taboo,
                                             taboo_duration, iteration);

      // restore solution in a greedy fashion
      size_t prev_size = current_solution.chosen_items.size();
      finish_solution(problem, current_solution);

      if (removed_item) {
        taboo[*removed_item] = iteration;

        free_items_.emplace(problem.items[*removed_item].relative_cost(),
                            *removed_item);
      }

      size_t cost = get_score(problem, current_solution);

      assert(evaluate(problem, current_solution).is_valid);

      const double delta =
          static_cast<double>(cost) - static_cast<double>(current_cost);

      if (delta >= 0 ||
          std::exp(delta / temperature) > random_accept(random_)) {
        current_cost = cost;

        if (current_cost > best_cost) {
          best_cost = current_cost;
          best_solution = current_solution;
        }
      } else {
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

      if (deadline.is_over()) {
        break;
      }

      if ((iteration + 1) % config.iterations_per_temperature == 0) {
        temperature *= config.alpha;
      }

      ++iteration;
    }

    std::println("  {} -> {}", get_score(problem, initial_solution), best_cost);

    return best_solution;
  }
};

}  // namespace knapsack
