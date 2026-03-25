#pragma once

#include <chrono>
#include <vector>

namespace coloring {

struct Graph {
  // for each vertex stores adjacency list
  std::vector<std::vector<size_t>> adjacent;
};

struct Solution {
  std::vector<size_t> colors;
};

struct EvaluationResult {
  size_t score;
  bool is_valid;
};

struct Statistics {
  size_t result;
  std::chrono::milliseconds duration;
};

}  // namespace coloring
