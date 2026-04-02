#pragma once

#include <algorithm>
#include <numeric>
#include <vector>

#include "TourStorage.h"
#include "tsp/Evaluator.h"
#include "tsp/Types.h"

namespace tsp {

inline bool all_different(std::vector<size_t> values) {
  return std::unordered_set(values.begin(), values.end()).size() ==
         values.size();
}

class LocalSearch3opt {
  constexpr static double tolerance = 1e-10;

  const Problem& problem;

  // Takes closest max_count points.
  static auto get_candidates_by_distance(const Problem& problem,
                                         const size_t max_count) {
    const size_t n = problem.points.size();
    std::vector<std::vector<size_t>> result(n);

    std::vector<size_t> heap;

    for (size_t i = 0; i < n; ++i) {
      auto proj = [i, &problem](size_t j) {
        return distance(problem.points[i], problem.points[j]);
      };

      for (size_t j = 0; j < n; ++j) {
        if (j == i) {
          continue;
        }

        heap.push_back(j);
        std::ranges::push_heap(heap, {}, proj);

        if (heap.size() > max_count) {
          std::ranges::pop_heap(heap, {}, proj);
          heap.pop_back();
        }
      }

      result[i] = heap;
      heap.clear();
    }

    return result;
  }

  double get_distance(size_t i, size_t j) const {
    return distance(problem.points[i], problem.points[j]);
  }

  bool try_improve(TourStorage& tour,
                   const std::vector<std::vector<size_t>>& candidates) {
    const size_t n = problem.points.size();

    for (size_t t1 = 0; t1 < n; ++t1) {
      for (const size_t direction : {0, 1}) {
        const size_t t2 = direction == 0 ? tour.succ(t1) : tour.pred(t1);

        for (const size_t t3 : candidates[t2]) {
          if (tour.is_neighbors(t3, t2)) {
            continue;
          }

          const size_t t4 = direction == 0 ? tour.pred(t3) : tour.succ(t3);
          assert(all_different({t1, t2, t3, t4}));

          double gain = get_distance(t1, t2) + get_distance(t3, t4) -
                        get_distance(t1, t4) - get_distance(t2, t3);

          if (gain < 0) {
            continue;
          }

          if (direction == 0) {
            tour.apply_2opt(t1, t4);
          } else {
            tour.apply_2opt(t2, t3);
          }

          // choose t5
          size_t t5;
          for (const size_t t5_candidate : candidates[t4]) {
            if (!tour.is_neighbors(t5_candidate, t4) && t5_candidate != t3) {
              t5 = t5_candidate;
              break;
            }
          }

          // choose t6

          if (gain > tolerance) {
            // std::println("  2-opt: {}, {}, {}, {}; gain = {}", t1, t2, t3,
            // t4, gain);

            if (direction == 0) {
              tour.apply_2opt(t1, t4);
            } else {
              tour.apply_2opt(t2, t3);
            }

            return true;
          }
        }
      }
    }

    return false;
  }

 public:
  explicit LocalSearch3opt(const Problem& problem) : problem(problem) {}

  Solution solve(const Solution& initial_solution) {
    TourStorage tour(initial_solution);
    auto candidates = get_candidates_by_distance(problem, 5);

    std::println("  score: {}", get_score(problem, tour.to_solution()));

    while (true) {
      assert(tour.is_valid());
      bool improved = try_improve(tour, candidates);

      if (!improved) {
        break;
      }

      std::println("  score: {}", get_score(problem, tour.to_solution()));
    }

    return tour.to_solution();
  }
};

}  // namespace tsp
