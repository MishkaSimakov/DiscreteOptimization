#pragma once

#include <cmath>
#include <iostream>
#include <random>

#include "GRASP.h"
#include "helpers/Hashers.h"
#include "helpers/Time.h"
#include "setcover/CoveringSetsPack.h"
#include "setcover/Evaluator.h"
#include "setcover/Types.h"

namespace setcover {

struct SimulatedAnnealingConfig {
  double relative_start_temperature = 1e-2;
  double relative_end_temperature = 1e-9;
  double alpha = 0.99;  // temperature change rate
  size_t iterations_per_temperature = 5;
  size_t iterations_per_move = 10;

  double taboo_duration_multiplier = 1;
};

class SimulatedAnnealing {
  const Problem& problem;
  const timing::Deadline deadline;

  const SimulatedAnnealingConfig sa_config;
  const GRASPConfig grasp_config;

  CoveringSetsPack pack_;

  // random
  size_t random_counter_{0};
  std::default_random_engine engine_;

  size_t random_index(size_t size) { return (++random_counter_ * 41) % size; }

  void finish_solution(const Problem& problem, Solution& solution) {
    for (size_t i = 0; i < problem.elements_count; ++i) {
      if (pack_.is_covered(i)) {
        continue;
      }

      auto sets = pack_.get_sets_covering(i);

      // set with small relative cost is more likely
      double sum = 0;
      for (const size_t set : sets) {
        if (pack_.get_set_state(set) == SetState::DEFAULT) {
          sum += 1. / pack_.get_relative_cost(set);
        }
      }

      double value = std::uniform_real_distribution<>(0, sum)(engine_);

      for (const size_t set : sets) {
        if (pack_.get_set_state(set) == SetState::DEFAULT) {
          value -= 1. / pack_.get_relative_cost(set);

          if (value <= 0) {
            solution.chosen_sets.push_back(set);
            pack_.include_set(set);

            break;
          }
        }
      }
    }
  }

  size_t remove_random_set(Solution& solution) {
    size_t index = random_index(solution.chosen_sets.size());
    size_t set_index = solution.chosen_sets[index];

    solution.chosen_sets.erase(solution.chosen_sets.begin() + index);
    pack_.default_set(set_index);

    return set_index;
  }

  Solution apply_sa(Solution initial_solution) {
    std::uniform_real_distribution<double> random_accept(0, 1);

    // square root of neighborhood size
    const size_t taboo_duration =
        sa_config.taboo_duration_multiplier *
        std::sqrt(static_cast<double>(initial_solution.chosen_sets.size()));

    // (set index, last iteration when it was removed)
    std::vector<size_t> taboo_list(problem.sets.size(), -1);

    Solution current_solution = initial_solution;
    size_t current_cost = get_score(problem, current_solution);

    Solution best_solution = current_solution;
    size_t best_cost = current_cost;

    double temperature = sa_config.relative_start_temperature *
                         static_cast<double>(current_cost);
    const double end_temperature =
        sa_config.relative_end_temperature * static_cast<double>(current_cost);
    size_t iteration = 0;

    // std::print("  {} -> ", current_cost);

    pack_.reset();
    for (const size_t set : current_solution.chosen_sets) {
      pack_.include_set(set);
    }

    while (temperature > end_temperature) {
      // remove random sets from the solution
      const size_t removed = remove_random_set(current_solution);

      if (taboo_list[removed] == -1 ||
          taboo_list[removed] + taboo_duration < iteration) {
        taboo_list[removed] = iteration;

        // restore solution feasibility
        const size_t prev_size = current_solution.chosen_sets.size();
        finish_solution(problem, current_solution);

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

            // std::print("{} -> ", new_cost);
            // std::cout << std::flush;
          }
        } else {
          // rollback changes
          for (size_t j = prev_size; j < current_solution.chosen_sets.size();
               ++j) {
            pack_.default_set(current_solution.chosen_sets[j]);
          }
          current_solution.chosen_sets.resize(prev_size);

          pack_.include_set(removed);
          current_solution.chosen_sets.push_back(removed);
        }
      } else {
        // rollback changes
        pack_.include_set(removed);
        current_solution.chosen_sets.push_back(removed);
      }

      if ((iteration + 1) % sa_config.iterations_per_temperature == 0) {
        temperature *= sa_config.alpha;
      }
      ++iteration;

      if (deadline.is_over()) {
        break;
      }
    }

    // std::println("done");

    return best_solution;
  }

 public:
  explicit SimulatedAnnealing(const Problem& problem, timing::Deadline deadline,
                              SimulatedAnnealingConfig sa_config,
                              GRASPConfig grasp_config)
      : problem(problem),
        deadline(deadline),
        sa_config(sa_config),
        grasp_config(grasp_config),
        pack_(problem) {}

  std::vector<Solution> solve() {
    GRASPConfig current_grasp_config = grasp_config;
    GRASP grasp(problem, current_grasp_config);

    std::vector<Solution> best_solutions = {grasp.solve()};
    size_t best_score = get_score(problem, best_solutions[0]);

    std::unordered_set<size_t> visited;

    size_t failed_iterations = 0;
    size_t successful_iterations = 0;

    while (!deadline.is_over()) {
      if (failed_iterations > successful_iterations &&
          current_grasp_config.temperature < 0.5) {
        std::println(
            "temperature = {}, iterations: failed = {}, successful = {}",
            current_grasp_config.temperature, failed_iterations,
            successful_iterations);
        current_grasp_config.temperature *= 1.1;
        failed_iterations = 0;

        grasp.set_config(current_grasp_config);
      }

      auto solution = grasp.solve();

      const size_t hash = unordered_vector_hash(solution.chosen_sets);
      auto [_, inserted] = visited.emplace(hash);

      if (inserted) {
        // improve solution using Simulated Annealing
        auto sa_improved = apply_sa(solution);

        const size_t new_score = get_score(problem, sa_improved);

        if (new_score <= best_score) {
          best_solutions.push_back(sa_improved);

          std::ranges::sort(sa_improved.chosen_sets);

          for (const size_t set : sa_improved.chosen_sets) {
            std::cout << "  " << set;
          }

          std::cout << std::endl;
        }

        if (new_score < best_score) {
          best_score = new_score;

          best_solutions.clear();
          best_solutions.push_back(std::move(sa_improved));

          std::println("  [!] new best: {}", best_score);
        }

        ++successful_iterations;
      } else {
        ++failed_iterations;
      }
    }

    return best_solutions;
  }
};

}  // namespace setcover
