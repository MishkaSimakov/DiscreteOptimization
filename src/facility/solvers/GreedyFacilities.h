#pragma once

#include <algorithm>
#include <cassert>
#include <random>
#include <ranges>
#include <unordered_set>

#include "facility/Types.h"
#include "utils/Accumulators.h"

namespace facility {

// Process facilities in order.
// Try to open next facility and assign customers to it, starting from the
// closest ones.
// 1. If customer is unassigned, assign him to the current facility
// 2. If customer is assigned, check if reassigning him would improve score
class GreedyFacilities {
  const Problem& problem;

  std::default_random_engine random_;

  Solution solve_with_order(const std::vector<size_t>& order) {
    const auto [n, d] = problem.shape();
    const auto& customers = problem.customers;
    const auto& facilities = problem.facilities;

    // -1 - customer is unassigned
    std::vector<size_t> result(d, -1);
    std::vector<size_t> old(d);
    std::vector<std::pair<double, size_t>> gains(d);

    // open first few facilities, so that all customers are assigned
    size_t opened_count = 0;
    size_t assigned_count = 0;

    for (const size_t i : order) {
      for (size_t j = 0; j < d; ++j) {
        gains[j] = {distance(customers[j].position, facilities[i].position), j};
      }

      std::ranges::sort(gains);

      // take customers while we can
      double total_demand = 0;

      for (const auto [gain, customer] : gains) {
        if (gain < 1e-10) {
          break;
        }

        if (result[customer] != -1 ||
            total_demand + customers[customer].demand >
                facilities[i].capacity) {
          continue;
        }

        result[customer] = i;
        total_demand += customers[customer].demand;
        ++assigned_count;
      }

      ++opened_count;

      if (assigned_count == d) {
        break;
      }
    }

    for (const size_t i : order | std::views::drop(assigned_count)) {
      // sort customers by gain
      for (size_t j = 0; j < d; ++j) {
        const double gain =
            distance(customers[j].position, facilities[result[j]].position) -
            distance(customers[j].position, facilities[i].position);

        gains[j] = {gain, j};
      }

      std::ranges::sort(gains, {}, [&](auto p) { return -p.first; });

      // take customers while we can
      double total_demand = 0;
      double total_gain = 0;

      for (const auto [gain, customer] : gains) {
        if (gain < 1e-10) {
          break;
        }

        if (total_demand + customers[customer].demand >
            facilities[i].capacity) {
          continue;
        }

        old[customer] = result[customer];
        result[customer] = i;

        total_demand += customers[customer].demand;
        total_gain += gain;
      }

      if (total_gain < facilities[i].cost) {
        // roll back
        for (size_t j = 0; j < d; ++j) {
          if (result[j] == i) {
            result[j] = old[j];
          }
        }
      }
    }

    return Solution{std::move(result)};
  }

 public:
  explicit GreedyFacilities(const Problem& problem) : problem(problem) {}

  Solution solve() {
    const auto [n, d] = problem.shape();

    std::vector<size_t> order(n);
    std::iota(order.begin(), order.end(), 0);

    double best_score = 1e10;
    Solution best_solution;

    for (size_t i = 0; i < 1000; ++i) {
      std::ranges::shuffle(order, random_);

      auto solution = solve_with_order(order);
      double score = get_score(problem, solution);

      if (score < best_score) {
        best_score = score;
        best_solution = std::move(solution);
      }
    }

    return best_solution;
  }
};

}  // namespace facility
