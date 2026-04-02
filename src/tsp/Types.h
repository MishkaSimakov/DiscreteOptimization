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

  double get_distance(size_t i, size_t j) const {
    return distance(points[i], points[j]);
  }
};

struct Solution {
  std::vector<size_t> next;
};

// Distance function between solutions.
// Defined as the number of edges that belong to the left tour but do not belong
// to the right tour (or vice versa, result would not change).
inline size_t distance(const Solution& left, const Solution& right) {
  assert(left.next.size() == right.next.size() &&
         "solutions must belong to the same problem");

  const size_t n = left.next.size();
  size_t result = 0;

  for (size_t i = 0; i < n; ++i) {
    if (left.next[i] != right.next[i] && i != right.next[left.next[i]]) {
      ++result;
    }
  }

  return result;
}

struct EvaluationResult {
  double score;
  bool is_valid;

  static EvaluationResult invalid() { return {.score = 0, .is_valid = false}; }
};

struct Statistics : EvaluationResult {
  std::chrono::milliseconds duration;

  Statistics(EvaluationResult evaluation, std::chrono::milliseconds duration)
      : EvaluationResult(evaluation), duration(duration) {}
};

}  // namespace tsp
