#pragma once
#include <cassert>
#include <vector>

#include "../Types.h"

namespace setcover {

class Greedy {
  static size_t argmax_set(const std::vector<double>& relative_costs) {
    size_t max_index = 0;

    for (size_t i = 1; i < relative_costs.size(); ++i) {
      if (relative_costs[i] > relative_costs[max_index]) {
        max_index = i;
      }
    }

    return max_index;
  }

  static bool is_zero(double value) { return std::abs(value) < 1e-10; }

 public:
  Greedy() = default;

  Solution solve(const Problem& problem) {
    size_t sets_count = problem.sets.size();

    std::vector<double> relative_costs(sets_count);
    std::vector<bool> is_covered(problem.elements_count, false);
    std::vector<size_t> result;

    for (size_t i = 0; i < sets_count; ++i) {
      relative_costs[i] = static_cast<double>(problem.sets[i].elements.size()) /
                          static_cast<double>(problem.sets[i].cost);
    }

    while (true) {
      size_t chosen_set = argmax_set(relative_costs);

      // quit if all elements are covered
      if (is_zero(relative_costs[chosen_set])) {
        break;
      }

      result.push_back(chosen_set);

      // update costs
      for (size_t element : problem.sets[chosen_set].elements) {
        if (is_covered[element]) {
          continue;
        }

        is_covered[element] = true;

        for (size_t i = 0; i < sets_count; ++i) {
          if (problem.sets[i].elements.contains(element)) {
            relative_costs[i] -= 1. / static_cast<double>(problem.sets[i].cost);
          }
        }
      }

      assert(is_zero(relative_costs[chosen_set]));
    }

    return Solution{std::move(result)};
  }
};

}  // namespace setcover
