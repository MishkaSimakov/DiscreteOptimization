#pragma once

#include <algorithm>
#include <numeric>
#include <vector>

#include "Candidates.h"
#include "TourStorage.h"
#include "tsp/Evaluator.h"
#include "tsp/Types.h"

namespace tsp {

class LocalSearch2opt {
  constexpr static double tolerance = 1e-10;

  const Problem& problem;
  const std::vector<std::vector<size_t>> candidates;

  // try_improve starts where it stopped in the previous iteration
  // this leads to HUGE time savings!
  size_t improvement_position{0};

  bool try_improve(TourStorage& tour) {
    const size_t n = problem.points.size();

    for (size_t i = 0; i < n; ++i) {
      const size_t t1 = improvement_position;

      for (const size_t direction : {0, 1}) {
        const size_t t2 = direction == 0 ? tour.succ(t1) : tour.pred(t1);

        for (const size_t t3 : candidates[t2]) {
          if (tour.is_neighbors(t3, t2)) {
            continue;
          }

          const size_t t4 = tour.get_2opt_node(t1, t2, t3);

          const double gain =
              problem.get_distance(t1, t2) + problem.get_distance(t3, t4) -
              problem.get_distance(t1, t4) - problem.get_distance(t2, t3);

          if (gain > tolerance) {
            // std::println("  2-opt: {}, {}, {}, {}; gain = {}", t1, t2, t3,
            // t4, gain);

            tour.apply_2opt(t1, t2, t3);

            return true;
          }
        }
      }

      improvement_position = (improvement_position + 1) % n;
    }

    return false;
  }

 public:
  LocalSearch2opt(const Problem& problem,
                  std::vector<std::vector<size_t>> candidates)
      : problem(problem), candidates(std::move(candidates)) {
    assert(candidates.size() == problem.points.size());
  }

  explicit LocalSearch2opt(const Problem& problem)
      : LocalSearch2opt(problem, get_candidates_by_distance(problem, 5)) {}

  Solution solve(const Solution& initial_solution) {
    TourStorage tour(initial_solution);

    while (true) {
      assert(tour.is_valid());
      bool improved = try_improve(tour);

      if (!improved) {
        break;
      }
    }

    return tour.to_solution();
  }
};

}  // namespace tsp
