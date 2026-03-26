#pragma once

#include <cmath>
#include <random>

#include "helpers/Hashers.h"
#include "helpers/Time.h"
#include "setcover/CoveringSetsPack.h"
#include "setcover/Evaluator.h"
#include "setcover/Types.h"

namespace setcover {

class SimulatedAnnealing {
  const timing::Deadline deadline;

  // random
  size_t random_counter_{0};
  std::default_random_engine engine_;

  size_t random_index(size_t size) { return (++random_counter_ * 41) % size; }

  void finish_solution(const Problem& problem, Solution& solution,
                       CoveringSetsPack& pack) {
    pack.reset();

    for (const size_t set_index : solution.chosen_sets) {
      pack.include_set(set_index);
    }

    for (size_t i = 0; i < problem.elements_count; ++i) {
      if (pack.is_covered(i)) {
        continue;
      }

      auto sets = pack.get_sets_covering(i);

      size_t default_count = 0;
      for (const size_t set : sets) {
        if (pack.get_set_state(set) == SetState::DEFAULT) {
          ++default_count;
        }
      }

      size_t index = random_index(default_count);

      for (const size_t set : sets) {
        if (pack.get_set_state(set) == SetState::DEFAULT) {
          if (index == 0) {
            solution.chosen_sets.push_back(set);
            pack.include_set(set);

            break;
          }

          --index;
        }
      }
    }
  }

  size_t hash_solution(const Solution& solution) {
    StreamUnorderedHasher hasher;

    for (const size_t set : solution.chosen_sets) {
      hasher << set;
    }

    return hasher.get_hash();
  }

  void remove_random_sets(Solution& solution, const size_t count) {
    double probability = static_cast<double>(count) /
                         static_cast<double>(solution.chosen_sets.size());

    std::erase_if(solution.chosen_sets, [&](size_t) {
      std::uniform_real_distribution<double> random_accept(0, 1);

      return random_accept(engine_) < probability;
    });
  }

 public:
  explicit SimulatedAnnealing(timing::Deadline deadline) : deadline(deadline) {}

  Solution solve(const Problem& problem, const Solution& initial_solution) {
    std::uniform_real_distribution<double> random_accept(0, 1);

    constexpr double relative_start_temperature = 1e-2;
    constexpr double relative_end_temperature = 1e-9;
    constexpr double alpha = 0.99;  // temperature change rate
    constexpr size_t removed_sets_count = 3;
    constexpr size_t iterations_per_temperature = 100;

    std::unordered_set<size_t> taboo_list;
    CoveringSetsPack pack(problem);

    Solution current_solution = initial_solution;
    size_t current_cost = get_score(problem, current_solution);

    Solution best_solution = initial_solution;
    size_t best_cost = current_cost;

    double temperature = relative_start_temperature * current_cost;
    const double end_temperature = relative_end_temperature * current_cost;
    size_t iteration = 0;

    std::print("  {} -> ", current_cost);

    while (temperature > end_temperature) {
      // remove random sets
      Solution new_solution = current_solution;

      remove_random_sets(new_solution, removed_sets_count);

      // restore solution in a greedy fashion
      finish_solution(problem, new_solution, pack);

      size_t hash = hash_solution(new_solution);
      auto [itr, inserted] = taboo_list.emplace(hash);

      if (inserted) {
        size_t new_cost = get_score(problem, new_solution);

        double delta =
            static_cast<double>(new_cost) - static_cast<double>(current_cost);

        // accept using simulated annealing algorithm
        if (delta <= 0 ||
            std::exp(-delta / temperature) > random_accept(engine_)) {
          current_solution = std::move(new_solution);
          current_cost = new_cost;

          if (current_cost < best_cost) {
            best_cost = current_cost;
            best_solution = current_solution;

            std::print("{} -> ", new_cost);
          }
        }
      }

      if ((iteration + 1) % iterations_per_temperature == 0) {
        temperature *= alpha;
      }
      ++iteration;

      if (deadline.is_over()) {
        break;
      }
    }

    std::print("done\n");

    return best_solution;
  }
};

}  // namespace setcover
