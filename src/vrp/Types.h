#pragma once

#include <chrono>
#include <vector>

#include "helpers/Geometry.h"

namespace vrp {

struct Customer {
  size_t demand;

  geom::Point<double> position;
};

struct Problem {
  size_t vehicles_count;
  size_t vehicle_capacity;

  std::vector<Customer> customers;

  static geom::Point<double> origin() { return {0, 0}; }
};

struct Solution {
  // For each vehicle i, routes[i] stores customers served by vehicle i in order
  std::vector<std::vector<size_t>> routes;
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

}  // namespace vrp
