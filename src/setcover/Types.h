#pragma once

#include <chrono>
#include <unordered_set>
#include <vector>

#include "helpers/Time.h"

namespace setcover {

struct CoveringSet {
  size_t cost;
  std::unordered_set<size_t> elements;
};

struct Problem {
  std::vector<CoveringSet> sets;

  size_t elements_count;
};

struct Solution {
  std::vector<size_t> chosen_sets;
};

struct EvaluationResult {
  size_t score;
  bool is_valid;
};

struct Statistics : EvaluationResult {
  timing::Duration duration;

  Statistics(EvaluationResult evaluation, timing::Duration duration)
      : EvaluationResult(evaluation), duration(duration) {}
};

}  // namespace setcover
