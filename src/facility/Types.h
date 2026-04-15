#pragma once

#include <chrono>
#include <cmath>
#include <vector>

namespace facility {

struct Point {
  double x;
  double y;
};

inline double distance(Point x, Point y) {
  const double dx = x.x - y.x;
  const double dy = x.y - y.y;

  return std::sqrt(dx * dx + dy * dy);
}

struct Facility {
  double cost;
  double capacity;
  Point position;
};

struct Customer {
  double demand;
  Point position;
};

struct Problem {
  std::vector<Facility> facilities;
  std::vector<Customer> customers;
};

struct Solution {
  // Assigned facility for each customer
  std::vector<size_t> facility;
};

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

}  // namespace facility
