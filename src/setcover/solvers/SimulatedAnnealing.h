#pragma once

#include <cmath>
#include <iostream>
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

  std::vector<size_t> remove_random_sets(Solution& solution,
                                         const size_t count) {
    std::vector<size_t> removed_sets;

    double probability = static_cast<double>(count) /
                         static_cast<double>(solution.chosen_sets.size());

    std::erase_if(solution.chosen_sets, [&](size_t set) {
      std::uniform_real_distribution<double> random_accept(0, 1);

      if (random_accept(engine_) < probability) {
        removed_sets.push_back(set);
        return true;
      }

      return false;
    });

    return removed_sets;
  }

 public:
  explicit SimulatedAnnealing(timing::Deadline deadline) : deadline(deadline) {}

  Solution solve(const Problem& problem, const Solution& initial_solution) {
    std::uniform_real_distribution<double> random_accept(0, 1);

    constexpr double relative_start_temperature = 1e-2;
    constexpr double relative_end_temperature = 1e-7;
    constexpr double alpha = 0.99;  // temperature change rate
    constexpr size_t removed_sets_count = 3;
    constexpr size_t iterations_per_temperature = 100;
    // square root of neighborhood size
    const size_t taboo_duration =
        std::pow(static_cast<double>(initial_solution.chosen_sets.size()),
                 static_cast<double>(removed_sets_count) / 2);

    // (move hash, last iteration when it was used)
    std::unordered_map<size_t, size_t> taboo_list;
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
      // remove random sets from the solution
      auto removed = remove_random_sets(current_solution, removed_sets_count);

      // restore solution feasibility
      size_t prev_size = current_solution.chosen_sets.size();
      finish_solution(problem, current_solution, pack);

      size_t hash = unordered_vector_hash(removed);
      auto [itr, inserted] = taboo_list.emplace(hash, iteration);

      bool applied_transformation = false;

      if (inserted || itr->second + taboo_duration < iteration) {
        itr->second = iteration;
        size_t new_cost = get_score(problem, current_solution);

        double delta =
            static_cast<double>(new_cost) - static_cast<double>(current_cost);

        // accept using simulated annealing algorithm
        if (delta <= 0 ||
            std::exp(-delta / temperature) > random_accept(engine_)) {
          current_cost = new_cost;

          if (current_cost < best_cost) {
            best_cost = current_cost;
            best_solution = current_solution;
          }

          std::print("{} -> ", new_cost);
          std::cout << std::flush;
          applied_transformation = true;
        }
      }

      if (!applied_transformation) {
        // rollback changes
        current_solution.chosen_sets.resize(prev_size);

        for (const size_t set : removed) {
          current_solution.chosen_sets.push_back(set);
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

    std::println("done");

    return best_solution;
  }
};

}  // namespace setcover
