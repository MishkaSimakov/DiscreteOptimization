#pragma once

#include <optional>
#include <random>

#include "HillClimber.h"
#include "SimulatedAnnealing.h"
#include "setcover/CoveringSetsPack.h"
#include "setcover/Types.h"

namespace setcover {

class GRASP {
  std::default_random_engine engine_;

  // Температура 1 - всегда выбираем случайное (не слишком плохое) множество.
  // Температура 0 - жадный алгоритм.
  const double temperature;

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
      if (coin(engine_) < temperature) {
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
      : temperature(temperature),
        quality_threshold(quality_threshold),
        simulated_annealing_config(config),
        deadline(deadline) {}

  Solution solve(const Problem& problem) {
    CoveringSetsPack pack(problem);
    Solution best_solution;
    size_t best_score = 1e10;  // infinity

    size_t iterations_cnt = 0;

    while (true) {
      ++iterations_cnt;

      if (deadline.is_over()) {
        break;
      }

      auto result = iteration(problem, pack, best_score);

      // improve solution using Simulated Annealing
      auto improved = SimulatedAnnealing(deadline, simulated_annealing_config)
                          .solve(problem, result.first);
      size_t cost = get_score(problem, improved);

      if (cost < best_score) {
        best_score = cost;
        best_solution = std::move(improved);
      }
    }

    // std::println("grasp iterations: {}", iterations_cnt);

    return best_solution;
  }
};

}  // namespace setcover
