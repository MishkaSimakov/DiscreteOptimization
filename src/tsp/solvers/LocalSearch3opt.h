#pragma once

#include <algorithm>
#include <numeric>
#include <vector>

#include "Candidates.h"
#include "TourStorage.h"
#include "tsp/Evaluator.h"
#include "tsp/Types.h"

namespace tsp {

inline void assert_all_different(std::vector<size_t> values) {
  std::unordered_map<size_t, size_t> map;

  for (size_t i = 0; i < values.size(); ++i) {
    auto [itr, inserted] = map.emplace(values[i], i);

    if (!inserted) {
      throw std::runtime_error(
          std::format("values[{}] == values[{}]", i, itr->second));
    }
  }
}

class LocalSearch3opt {
  constexpr static double tolerance = 1e-10;

  const Problem& problem;
  const std::vector<std::vector<size_t>> candidates;

  size_t improvement_position_{0};

  bool try_improve(TourStorage<>& tour) {
    const size_t n = problem.points.size();

    for (size_t i = 0; i < n; ++i) {
      const size_t t1 = improvement_position_;

      for (const size_t direction : {0, 1}) {
        const size_t t2 = direction == 0 ? tour.succ(t1) : tour.pred(t1);

        for (const size_t t3 : candidates[t2]) {
          if (tour.is_neighbors(t3, t2)) {
            continue;
          }

          const size_t t4 = direction == 0 ? tour.pred(t3) : tour.succ(t3);
          assert_all_different({t1, t2, t3, t4});

          const double gain_2opt =
              problem.get_distance(t1, t2) + problem.get_distance(t3, t4) -
              problem.get_distance(t1, t4) - problem.get_distance(t2, t3);

          if (gain_2opt <= tolerance) {
            continue;
          }

          // std::println("  2-opt: {}, {}, {}, {}; gain = {}", t1, t2, t3, t4,
          // gain_2opt);

          tour.apply_2opt(t1, t2, t3);

          // choose t5
          std::optional<size_t> t5;
          for (const size_t t5_candidate : candidates[t4]) {
            if (!tour.is_neighbors(t5_candidate, t4) &&
                !tour.is_neighbors(t5_candidate, t2) && t5_candidate != t2) {
              t5 = t5_candidate;
              break;
            }
          }

          if (!t5) {
            return true;
          }

          // choose t6
          const size_t t6 = tour.get_2opt_node(t1, t4, *t5);

          if (t6 == t1 || t6 == t2 || t6 == t3 || t6 == t4 || t6 == *t5) {
            return true;
          }

          double gain_3opt =
              problem.get_distance(t1, t2) + problem.get_distance(t3, t4) +
              problem.get_distance(*t5, t6) - problem.get_distance(t1, t6) -
              problem.get_distance(t2, t3) - problem.get_distance(t4, *t5);

          // apply 3-opt only if gain is better than 2-opt
          if (gain_3opt < gain_2opt) {
            return true;
          }

          assert_all_different({t1, t2, t3, t4, *t5, t6});

          // std::println("  3-opt: {}, {}, {}, {}, {}, {}; gain = {}", t1, t2,
          // t3, t4, *t5, t6, gain_3opt);

          tour.apply_2opt(t1, t4, *t5);

          return true;
        }
      }

      improvement_position_ = (improvement_position_ + 1) % n;
    }

    return false;
  }

 public:
  LocalSearch3opt(const Problem& problem,
                  std::vector<std::vector<size_t>> candidates)
      : problem(problem), candidates(std::move(candidates)) {
    assert(this->candidates.size() == problem.points.size());
  }

  explicit LocalSearch3opt(const Problem& problem)
      : LocalSearch3opt(problem, get_candidates_by_distance(problem, 5)) {}

  Solution solve(const Solution& initial_solution) {
    TourStorage tour(initial_solution);

    // std::println("  score: {}", get_score(problem, tour.to_solution()));

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

