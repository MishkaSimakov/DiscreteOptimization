#pragma once

#include <optional>
#include <random>

#include "HillClimber2.h"
#include "SimulatedAnnealing.h"
#include "setcover/CoveringSetsPack.h"
#include "setcover/Types.h"

namespace setcover {

class GRASP {
  std::default_random_engine engine_;

  // Температура 1 - всегда выбираем случайное (не слишком плохое) множество.
  // Температура 0 - жадный алгоритм.
  double temperature_;

  // Порог на выбор случайного множества. Пусть множество, которое выбрал бы
  // жадный алгоритм имеет относительную стоимость l. Тогда случайное множество
  // будет выбираться так, чтобы его относительная стоимость была хотя бы l *
  // quality_threshold_
  const double quality_threshold;

  const SimulatedAnnealingConfig simulated_annealing_config;

  const timing::Deadline deadline;

  static std::pair<size_t, double> argmax_set(
      const std::vector<CoveringSet>& sets) {
    size_t max_index = 0;
    double max_value = 0;

    for (size_t i = 0; i < sets.size(); ++i) {
      double value = static_cast<double>(sets[i].elements.size()) /
                     static_cast<double>(sets[i].cost);

      if (max_value < value) {
        max_value = value;
        max_index = i;
      }
    }

    return {max_index, max_value};
  }

  static std::vector<size_t> get_suitable_sets(
      const std::vector<CoveringSet>& sets, double threshold) {
    std::vector<size_t> result;

    for (size_t i = 0; i < sets.size(); ++i) {
      double value = static_cast<double>(sets[i].elements.size()) /
                     static_cast<double>(sets[i].cost);

      if (value >= threshold) {
        result.push_back(i);
      }
    }

    return result;
  }

  std::pair<Solution, size_t> iteration(const Problem& problem,
                                        CoveringSetsPack& pack,
                                        size_t best_score) {
    std::uniform_real_distribution<double> coin(0, 1);
    std::vector<size_t> result;

    size_t current_score = 0;

    pack.reset();

    while (true) {
      auto greedy_set = pack.max_cost_set();

      if (!greedy_set) {
        break;
      }

      size_t chosen_set = greedy_set->first;
      if (coin(engine_) < temperature_) {
        auto suitable_sets =
            pack.get_default_sets(greedy_set->second * quality_threshold);

        std::uniform_int_distribution<size_t> dist(0, suitable_sets.size() - 1);
        chosen_set = suitable_sets[dist(engine_)];
      }

      current_score += problem.sets[chosen_set].cost;
      result.push_back(chosen_set);
      pack.include_set(chosen_set);
    }

    return std::pair{Solution{std::move(result)}, current_score};
  }

 public:
  explicit GRASP(double temperature, double quality_threshold,
                 timing::Deadline deadline, SimulatedAnnealingConfig config)
      : temperature_(temperature),
        quality_threshold(quality_threshold),
        simulated_annealing_config(config),
        deadline(deadline) {}

  Solution solve(const Problem& problem) {
    CoveringSetsPack pack(problem);
    Solution best_solution;
    size_t best_score = 1e10;  // infinity

    std::unordered_set<size_t> visited;

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

      auto result = iteration(problem, pack, best_score);

      size_t hash = unordered_vector_hash(result.first.chosen_sets);
      auto [_, inserted] = visited.emplace(hash);

      if (inserted) {
        // improve solution using Simulated Annealing
        auto sa_improved =
            SimulatedAnnealing(deadline, simulated_annealing_config)
                .solve(problem, result.first);
        size_t score_after_sa = get_score(problem, sa_improved);

        auto hc_improved = HillClimber2(deadline).solve(problem, sa_improved);
        size_t cost = get_score(problem, hc_improved);

        // if (score_after_sa != cost) {
          // std::println("  {} -> {}", get_score(problem, result.first), cost);
        // }

        if (cost < best_score) {
          std::println("  {}", cost);
          best_score = cost;
          best_solution = std::move(hc_improved);
        }

        ++successful_iterations;
      } else {
        // std::println("skip");
        ++failed_iterations;
      }

      if (deadline.is_over()) {
        break;
      }
    }

    std::println("iterations: successful = {}, failed = {}",
                 successful_iterations, failed_iterations);

    return best_solution;
  }
};

}  // namespace setcover
