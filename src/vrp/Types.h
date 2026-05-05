#pragma once

#include <chrono>
#include <vector>

#include "helpers/Geometry.h"
#include "helpers/Time.h"

namespace vrp {

struct Customer {
  size_t demand;

  geom::Point<double> position;
};

struct Problem {
  geom::Point<double> origin;

  size_t vehicles_count;
  size_t vehicle_capacity;

  std::vector<Customer> customers;
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
  timing::Duration duration;

  Statistics(EvaluationResult evaluation, timing::Duration duration)
      : EvaluationResult(evaluation), duration(duration) {}
};

}  // namespace vrp
