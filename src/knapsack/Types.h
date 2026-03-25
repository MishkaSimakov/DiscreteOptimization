#pragma once

#include <vector>

namespace knapsack {

struct Item {
  size_t weight;
  size_t cost;
};

struct Problem {
  std::vector<Item> items;
  size_t max_weight;
};

struct Solution {
  std::vector<size_t> chosen_items;
};

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
