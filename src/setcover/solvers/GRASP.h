#pragma once

#include <optional>
#include <random>

#include "setcover/CoveringSetsPack.h"
#include "setcover/Types.h"

namespace setcover {

struct GRASPConfig {
  // Температура 1 - всегда выбираем случайное (не слишком плохое) множество.
  // Температура 0 - жадный алгоритм.
  double temperature;

  // Порог на выбор случайного множества. Пусть множество, которое выбрал бы
  // жадный алгоритм имеет относительную стоимость l. Тогда случайное множество
  // будет выбираться так, чтобы его относительная стоимость была хотя бы l *
  // quality_threshold_
  double quality_threshold;
};

class GRASP {
  const Problem& problem;
  GRASPConfig config_;

  CoveringSetsPack pack_;
  std::default_random_engine random_;

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

 public:
  explicit GRASP(const Problem& problem, GRASPConfig config)
      : problem(problem), config_(config), pack_(problem) {}

  void set_config(GRASPConfig config) { config_ = config; }

  Solution solve() {
    std::uniform_real_distribution<double> coin(0, 1);
    std::vector<size_t> result;

    size_t current_score = 0;

    pack_.reset();

    while (true) {
      auto greedy_set = pack_.max_cost_set();

      if (!greedy_set) {
        break;
      }

      size_t chosen_set = greedy_set->first;
      if (coin(random_) < config_.temperature) {
        chosen_set = pack_.get_random_default(
            greedy_set->second * config_.quality_threshold, random_);
      }

      current_score += problem.sets[chosen_set].cost;
      result.push_back(chosen_set);
      pack_.include_set(chosen_set);
    }

    return Solution{std::move(result)};
  }
};

}  // namespace setcover
