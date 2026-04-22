#pragma once

#include <algorithm>
#include <cassert>
#include <print>
#include <random>
#include <unordered_set>

#include "HillClimber.h"
#include "SimulatedAnnealing.h"
#include "helpers/Hashers.h"
#include "knapsack/Evaluator.h"
#include "knapsack/Types.h"

namespace knapsack {

class GRASP {
  std::default_random_engine random_;

  // Температура 1 - всегда выбираем случайное (не слишком плохое) множество.
  // Температура 0 - жадный алгоритм.
  double temperature_;

  const timing::Deadline deadline;

  std::unordered_set<size_t> visited_solutions_;

  Solution iteration(
      const Problem& problem,
      const std::vector<std::pair<double, size_t>>& relative_costs) {
    std::uniform_real_distribution<double> coin(0, 1);
    std::vector<size_t> result;

    size_t current_weight = 0;
    size_t current_cost = 0;

    for (auto [_, id] : relative_costs) {
      if (coin(random_) < temperature_) {
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
  explicit GRASP(double temperature, timing::Deadline deadline)
      : temperature_(temperature), deadline(deadline) {}

  Solution solve(const Problem& problem) {
    std::vector<std::pair<double, size_t>> relative_costs(problem.items.size());
    for (size_t i = 0; i < problem.items.size(); ++i) {
      relative_costs[i] = {problem.items[i].relative_cost(), i};
    }

    std::ranges::sort(relative_costs, {}, [](std::pair<double, size_t> item) {
      return -item.first;
    });

    Solution best_solution;
    size_t best_score = 0;

    size_t iterations_cnt = 0;

    size_t failed_iterations = 0;
    size_t successful_iterations = 0;

    while (true) {
      if (failed_iterations > successful_iterations && temperature_ < 0.5) {
        std::println(
            "temperature = {}, iterations: failed = {}, successful = {}",
            temperature_, failed_iterations, successful_iterations);
        temperature_ *= 1.1;
        failed_iterations = 0;
      }

      ++iterations_cnt;

      if (deadline.is_over()) {
        break;
      }

      auto solution = iteration(problem, relative_costs);

      size_t hash = unordered_vector_hash(solution.chosen_items);
      auto [_, inserted] = visited_solutions_.emplace(hash);

      if (!inserted) {
        ++failed_iterations;
        continue;
      }

      ++successful_iterations;

      solution = HillClimber(deadline).solve(problem, solution);

      auto evaluation = evaluate(problem, solution);
      assert(evaluation.is_valid);

      if (evaluation.score > best_score) {
        best_score = evaluation.score;
        best_solution = std::move(solution);

        std::println("  new best: {}", best_score);
      }
    }

    std::println("iterations: {}", iterations_cnt);

    return best_solution;
  }
};

}  // namespace knapsack
