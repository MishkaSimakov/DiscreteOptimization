#pragma once
#include <algorithm>
#include <cassert>
#include <print>
#include <random>

#include "knapsack/Evaluator.h"
#include "knapsack/Types.h"

namespace knapsack {

class GRASP {
  std::default_random_engine engine_;

  // Температура 1 - всегда выбираем случайное (не слишком плохое) множество.
  // Температура 0 - жадный алгоритм.
  double temperature_;

  std::chrono::milliseconds time_limit_;

  Solution iteration(
      const Problem& problem,
      const std::vector<std::pair<double, size_t>>& relative_costs) {
    std::uniform_real_distribution<double> coin(0, 1);
    std::vector<size_t> result;

    size_t current_weight = 0;
    size_t current_cost = 0;

    for (auto [_, id] : relative_costs) {
      if (coin(engine_) < temperature_) {
        continue;
      }

      if (current_weight + problem.items[id].weight > problem.max_weight) {
        continue;
      }

      current_weight += problem.items[id].weight;
      current_cost += problem.items[id].cost;

      result.push_back(id);
    }

    return Solution{std::move(result)};
  }

 public:
  explicit GRASP(double temperature, std::chrono::milliseconds time_limit)
      : temperature_(temperature), time_limit_(time_limit) {}

  Solution solve(const Problem& problem) {
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::pair<double, size_t>> relative_costs(problem.items.size());
    for (size_t i = 0; i < problem.items.size(); ++i) {
      auto item = problem.items[i];

      relative_costs.emplace_back(
          static_cast<double>(item.cost) / static_cast<double>(item.weight), i);
    }

    std::ranges::sort(relative_costs, {}, [](std::pair<double, size_t> item) {
      return -item.first;
    });

    Solution best_solution;
    size_t best_score = 0;

    size_t iterations_cnt = 0;

    while (true) {
      ++iterations_cnt;

      auto current_time = std::chrono::high_resolution_clock::now();

      if (duration_cast<std::chrono::milliseconds>(current_time - start_time) >
          time_limit_) {
        break;
      }

      auto solution = iteration(problem, relative_costs);

      auto evaluation = evaluate(problem, solution);
      assert(evaluation.is_valid);

      if (evaluation.score > best_score) {
        best_score = evaluation.score;
        best_solution = std::move(solution);
      }
    }

    return best_solution;
  }
};

}  // namespace knapsack
