#pragma once

#include <print>
#include <random>

#include "GRASP.h"
#include "helpers/Time.h"
#include "setcover/CoveringSetsPack.h"
#include "setcover/Evaluator.h"
#include "setcover/Types.h"

namespace setcover {

class Probability {
  const Problem& problem;
  const timing::Deadline deadline;

 public:
  explicit Probability(const Problem& problem, timing::Deadline deadline)
      : problem(problem), deadline(deadline) {}

  Solution solve() {
    Problem current_problem = problem;

    std::vector<size_t> current_sets_map(problem.sets.size());
    std::iota(current_sets_map.begin(), current_sets_map.end(), 0);
    std::vector<size_t> current_solution;

    size_t iteration = 1;

    while (true) {
      const GRASPConfig config{
          .temperature = 0.5,
          .quality_threshold = 0.5,
      };

      GRASP grasp(current_problem, config);

      std::vector<double> weights(current_problem.sets.size(), 0);

      for (size_t i = 0; i < 500 * std::sqrt(iteration); ++i) {
        const auto solution = grasp.solve();
        const size_t score = get_score(current_problem, solution);

        for (const size_t set : solution.chosen_sets) {
          weights[set] += 1. / static_cast<double>(score - 166);
        }
      }

      std::vector<size_t> order(current_problem.sets.size());
      std::iota(order.begin(), order.end(), 0);
      std::ranges::sort(order, {}, [&](size_t i) { return -weights[i]; });

      // for (const size_t i : order) {
      // std::println("#{}: {}", i, weights[i]);
      // }

      // take first k sets, update problem
      CoveringSetsPack pack(current_problem);

      for (size_t i = 0; i < 1; ++i) {
        pack.include_set(order[i]);
        current_solution.push_back(current_sets_map[order[i]]);
      }

      size_t elements_count = 0;
      std::vector<size_t> elements_mapping(current_problem.elements_count, -1);

      for (size_t i = 0; i < current_problem.elements_count; ++i) {
        if (!pack.is_covered(i)) {
          elements_mapping[i] = elements_count;
          ++elements_count;
        }
      }

      if (elements_count == 0) {
        // solved
        break;
      }

      std::vector<CoveringSet> sets;
      for (size_t i = 0; i < current_problem.sets.size(); ++i) {
        if (pack.get_elements_count(i) != 0) {
          CoveringSet new_set;
          new_set.cost = current_problem.sets[i].cost;

          for (const size_t element : current_problem.sets[i].elements) {
            if (elements_mapping[element] != -1) {
              new_set.elements.insert(elements_mapping[element]);
            }
          }

          current_sets_map[sets.size()] = current_sets_map[i];
          sets.push_back(std::move(new_set));
        }
      }

      current_problem = {
          .elements_count = elements_count,
          .sets = std::move(sets),
      };

      std::println("  new sets count = {}, new elements count = {}",
                   current_problem.sets.size(), current_problem.elements_count);

      ++iteration;
    }

    std::println("score = {}", get_score(problem, Solution{current_solution}));

    return Solution{current_solution};
  }
};

}  // namespace setcover
