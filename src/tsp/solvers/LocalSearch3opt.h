#pragma once

#include <algorithm>
#include <numeric>
#include <vector>

#include "Candidates.h"
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

          double gain =
              problem.get_distance(t1, t2) + problem.get_distance(t3, t4) -
              problem.get_distance(t1, t4) - problem.get_distance(t2, t3);

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
