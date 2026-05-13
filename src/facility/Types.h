#pragma once

#include <chrono>
#include <cmath>
#include <vector>

#include "helpers/Geometry.h"
#include "helpers/Time.h"

namespace facility {

struct Facility {
  double cost;
  double capacity;
  geom::Point<double> position;
};

struct Customer {
  double demand;
  geom::Point<double> position;
};

struct Problem {
  std::vector<Facility> facilities;
  std::vector<Customer> customers;

  std::pair<size_t, size_t> shape() const {
    return {facilities.size(), customers.size()};
  }
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
  timing::Duration duration;

  Statistics(EvaluationResult evaluation, timing::Duration duration)
      : EvaluationResult(evaluation), duration(duration) {}
};

}  // namespace facility
