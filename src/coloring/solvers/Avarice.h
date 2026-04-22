#pragma once

#include <algorithm>
#include <numeric>
#include <random>

#include "coloring/Types.h"
#include "helpers/Time.h"

namespace coloring {

// Avarice is a fancy word for greed. This class is a more sophisticated
// implementation of greedy algorithm.
class Avarice {
  const Graph& problem;
  const timing::Deadline deadline;

  std::default_random_engine random_;

  void find_colors(const std::vector<size_t>& order,
                   std::vector<size_t>& colors,
                   std::vector<bool>& occupied_buffer) {
    size_t n = problem.adjacent.size();

    std::fill_n(colors.begin(), n, n);
    std::fill_n(occupied_buffer.begin(), n + 1, false);

    for (size_t i : order) {
      for (size_t adj : problem.adjacent[i]) {
        occupied_buffer[colors[adj]] = true;
      }

      for (size_t j = 0; j < n; ++j) {
        if (!occupied_buffer[j]) {
          colors[i] = j;
          break;
        }
      }

      for (size_t adj : problem.adjacent[i]) {
        occupied_buffer[colors[adj]] = false;
      }
    }
  }

  size_t get_colors_count(const std::vector<size_t>& colors) {
    size_t result = 0;
    for (size_t i : colors) {
      result = std::max(result, i + 1);
    }

    return result;
  }

  void update_order_reverse(const std::vector<size_t>& colors,
                            std::vector<size_t>& order) {
    std::ranges::sort(order, {},
                      [&](size_t i) { return -static_cast<int>(colors[i]); });
  }

  void update_order_largest_first(const std::vector<size_t>& colors,
                                  std::vector<size_t>& order) {
    std::vector<size_t> counts(get_colors_count(colors), 0);
    for (size_t i : colors) {
      ++counts[i];
    }

    std::ranges::sort(order, {}, [&](size_t i) {
      return std::pair{-static_cast<int>(counts[colors[i]]), colors[i]};
    });
  }

  void update_order_random(const std::vector<size_t>& colors,
                           std::vector<size_t>& order) {
    std::vector<size_t> new_order(get_colors_count(colors));
    std::iota(new_order.begin(), new_order.end(), 0);
    std::ranges::shuffle(new_order, random_);

    std::ranges::sort(order, {},
                      [&](size_t i) { return new_order[colors[i]]; });
  }

  void update_order(const std::vector<size_t>& colors,
                    std::vector<size_t>& order) {
    std::uniform_int_distribution<int> distr(0, 12);

    int decision = distr(random_);
    if (decision < 5) {
      update_order_largest_first(colors, order);
    } else if (decision < 10) {
      update_order_reverse(colors, order);
    } else {
      update_order_random(colors, order);
    }
  }

 public:
  Avarice(const Graph& problem, timing::Deadline deadline)
      : problem(problem), deadline(deadline) {}

  Solution solve(Solution initial_solution) {
    const size_t n = problem.adjacent.size();

    std::vector<size_t> order(n);
    std::iota(order.begin(), order.end(), 0);

    std::vector<size_t> colors = std::move(initial_solution.colors);
    std::vector<bool> occupied_buffer(n + 1, false);

    while (!deadline.is_over()) {
      update_order(colors, order);

      find_colors(order, colors, occupied_buffer);
    }

    return Solution{std::move(colors)};
  }
};

}  // namespace coloring
