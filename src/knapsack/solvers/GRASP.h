#pragma once
#include <random>

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
  double quality_threshold_;

  std::chrono::milliseconds time_limit_;

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

  // Solution iteration(const Problem& problem) {
  //   size_t sets_count = problem.sets.size();
  //
  //   std::uniform_real_distribution<double> coin(0, 1);
  //
  //   std::vector<size_t> result;
  //
  //   // копируем все множества, так как далее из них будут убираться уже
  //   покрытые
  //   // элементы
  //   auto sets = problem.sets;
  //
  //   while (true) {
  //     auto [greedy_set, greedy_cost] = argmax_set(sets);
  //
  //     size_t chosen_set = greedy_set;
  //     if (coin(engine_) < temperature_) {
  //       auto suitable_sets =
  //           get_suitable_sets(sets, greedy_cost * quality_threshold_);
  //
  //       std::uniform_int_distribution<size_t> dist(0, suitable_sets.size() -
  //       1); chosen_set = suitable_sets[dist(engine_)];
  //     }
  //
  //     if (sets[chosen_set].elements.empty()) {
  //       break;
  //     }
  //
  //     result.push_back(chosen_set);
  //
  //     for (size_t i = 0; i < sets_count; ++i) {
  //       if (i == chosen_set) {
  //         continue;
  //       }
  //
  //       for (size_t element : sets[chosen_set].elements) {
  //         sets[i].elements.erase(element);
  //       }
  //     }
  //     sets[chosen_set].elements.clear();
  //   }
  //
  //   return Solution{std::move(result)};
  // }
  std::optional<std::pair<Solution, size_t>> iteration(const Problem& problem,
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
            pack.get_covering_sets(greedy_set->second * quality_threshold_);

        std::uniform_int_distribution<size_t> dist(0, suitable_sets.size() - 1);
        chosen_set = suitable_sets[dist(engine_)];
      }

      current_score += problem.sets[chosen_set].cost;
      result.push_back(chosen_set);
      pack.cover_set(chosen_set);

      if (current_score > best_score) {
        return std::nullopt;
      }
    }

    return std::pair{Solution{std::move(result)}, current_score};
  }

 public:
  explicit GRASP(double temperature, double quality_threshold,
                 std::chrono::milliseconds time_limit)
      : temperature_(temperature),
        quality_threshold_(quality_threshold),
        time_limit_(time_limit) {}

  Solution solve(const Problem& problem) {
    auto start_time = std::chrono::high_resolution_clock::now();

    CoveringSetsPack pack(problem);
    Solution best_solution;
    size_t best_score = 1e10;  // infinity

    size_t iterations_cnt = 0;

    while (true) {
      ++iterations_cnt;

      auto current_time = std::chrono::high_resolution_clock::now();

      if (duration_cast<std::chrono::milliseconds>(current_time - start_time) >
          time_limit_) {
        break;
      }

      auto result = iteration(problem, pack, best_score);

      if (!result) {
        continue;
      }

      if (result->second < best_score) {
        best_score = result->second;
        best_solution = std::move(result->first);
      }
    }

    std::println("grasp iterations: {}", iterations_cnt);

    return best_solution;
  }
};

}  // namespace setcover
