#pragma once

#include <vector>

#include "utils/String.h"

namespace knapsack {

struct Item {
  size_t weight;
  size_t cost;

  double relative_cost() const {
    return static_cast<double>(cost) / static_cast<double>(weight);
  }
};

struct Problem {
  std::vector<Item> items;
  size_t max_weight;
};

struct Solution {
  std::vector<size_t> chosen_items;
};

inline std::ostream& operator<<(std::ostream& os, const Solution& solution) {
  auto serialized =
      solution.chosen_items |
      std::views::transform([](size_t item) { return std::to_string(item); });

  os << "(" << str::join(serialized, " ") << ")";

  return os;
}

struct EvaluationResult {
  size_t score;
  bool is_valid;
};

struct Statistics : EvaluationResult {
  std::chrono::milliseconds duration;

  Statistics(EvaluationResult evaluation, std::chrono::milliseconds duration)
      : EvaluationResult(evaluation), duration(duration) {}
};

}  // namespace knapsack
