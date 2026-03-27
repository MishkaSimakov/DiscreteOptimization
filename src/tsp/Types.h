#pragma once

#include <chrono>
#include <vector>

namespace tsp {

struct Point {
  size_t x;
  size_t y;
};

inline double distance(Point x, Point y) {
  const double dx = static_cast<double>(x.x) - static_cast<double>(y.x);
  const double dy = static_cast<double>(x.y) - static_cast<double>(y.y);

  return std::sqrt(dx * dx + dy * dy);
}

struct Problem {
  std::vector<Point> points;
};

struct Solution {
  std::vector<size_t> order;
};

struct EvaluationResult {
  double score;
  bool is_valid;
};

struct Statistics : EvaluationResult {
  std::chrono::milliseconds duration;

  Statistics(EvaluationResult evaluation, std::chrono::milliseconds duration)
      : EvaluationResult(evaluation), duration(duration) {}
};

}  // namespace tsp
