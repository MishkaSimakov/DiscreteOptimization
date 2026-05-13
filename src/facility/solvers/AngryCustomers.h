#pragma once

#include <cassert>
#include <random>

#include "facility/Types.h"
#include "helpers/Random.h"
#include "helpers/Time.h"
#include "utils/Accumulators.h"

namespace facility {

struct GeneticsParameters {
  size_t population_size;
  double mutation_rate;
  double similarity_replacement_threshold;
};

template <typename Improver>
class AngryCustomers {
  const Problem& problem;
  const timing::Deadline deadline;

  const GeneticsParameters params;

  Improver improver_;
  std::default_random_engine random_;

  static std::vector<size_t> get_closest(const Problem& problem) {
    const auto [n, d] = problem.shape();

    std::vector<size_t> closest(d);

    for (size_t i = 0; i < d; ++i) {
      ArgMinimum<double, std::less<>> min_distance;

      for (size_t j = 0; j < n; ++j) {
        min_distance.record(j, distance(problem.customers[i].position,
                                        problem.facilities[j].position));
      }

      closest[i] = min_distance->index;
    }

    return closest;
  }

  std::vector<bool> get_initial_individual() {
    const auto [n, d] = problem.shape();

    std::vector<bool> result(n);

    for (size_t i = 0; i < n; ++i) {
      result[i] = rnd::bernoulli(0.1, random_);
    }

    return result;
  }

  // Returns fully grown individual and assigned facility for each customer
  std::pair<std::vector<bool>, std::vector<size_t>> grow(
      std::vector<bool> individual) {
    const auto [n, d] = problem.shape();

    std::vector<size_t> result(d, 0);
    std::vector<double> current_demand(n, 0);

    for (size_t i = 0; i < d; ++i) {
      // try to assign facility without opening a new one
      ArgMinimum<double, std::less<>> min_cost;

      for (size_t j = 0; j < n; ++j) {
        if (!individual[j]) {
          continue;
        }

        if (current_demand[j] + problem.customers[i].demand >
            problem.facilities[j].capacity) {
          continue;
        }

        const double cost = distance(problem.customers[i].position,
                                     problem.facilities[j].position);

        min_cost.record(j, cost);
      }

      if (min_cost.has_value()) {
        result[i] = min_cost->index;
        current_demand[result[i]] += problem.customers[i].demand;

        continue;
      }

      // have to open new facility
      for (size_t j = 0; j < n; ++j) {
        if (current_demand[j] + problem.customers[i].demand >
            problem.facilities[j].capacity) {
          continue;
        }

        const double cost = distance(problem.customers[i].position,
                                     problem.facilities[j].position);

        min_cost.record(j, cost);
      }

      individual[min_cost->index] = true;
      result[i] = min_cost->index;
      current_demand[result[i]] += problem.customers[i].demand;
    }

    result = improver_.improve(std::move(result), current_demand);

    return {std::move(individual), std::move(result)};
  }

  std::vector<bool> crossover(const std::vector<bool>& left,
                              const std::vector<bool>& right) {
    const auto [n, d] = problem.shape();
    std::vector<bool> result(n);

    for (size_t i = 0; i < n; ++i) {
      if (left[i] && right[i]) {
        result[i] = true;
      } else if (!left[i] && !right[i]) {
        result[i] = false;
      } else {
        result[i] = rnd::bernoulli(0.5, random_);
      }
    }

    return result;
  }

  std::vector<bool> mutation(std::vector<bool> individual) {
    for (size_t i = 0; i < individual.size(); ++i) {
      if (rnd::bernoulli(0.05, random_)) {
        individual[i] = !individual[i];
      }
    }

    // size_t index = rnd::index(individual.size(), random_);
    // individual[index] = !individual[index];

    return individual;
  }

  size_t find_replaced(
      const std::pair<double, std::vector<bool>>& replacement,
      const std::vector<std::pair<double, std::vector<bool>>>& population) {
    double best_score = 1e10;  // infinity

    ArgMinimum<double> min_distance;

    for (size_t i = 0; i < population.size(); ++i) {
      best_score = std::min(best_score, population[i].first);

      min_distance.record(
          i, get_distance(replacement.second, population[i].second));
    }

    if (min_distance->min < params.similarity_replacement_threshold) {
      // similarity-based replacement
      const size_t closest_index = min_distance->index;

      if (std::abs(population[closest_index].first - best_score) > 1e-10 ||
          replacement.first < population[closest_index].first) {
        return closest_index;
      }
    }

    // return the worst one
    ArgMaximum<double, std::less<>> worst_score;

    for (size_t i = 0; i < population.size(); ++i) {
      worst_score.record(i, population[i].first);
    }

    return worst_score->index;
  }

  static size_t get_distance(const std::vector<bool>& left,
                             const std::vector<bool>& right) {
    size_t result = 0;

    for (size_t i = 0; i < left.size(); ++i) {
      if (left[i] != right[i]) {
        ++result;
      }
    }

    return result;
  }

  static double get_population_diversity(
      const std::vector<std::pair<double, std::vector<bool>>>& population) {
    size_t sum = 0;
    size_t count = 0;

    for (size_t i = 0; i < population.size(); ++i) {
      for (size_t j = i + 1; j < population.size(); ++j) {
        sum += get_distance(population[i].second, population[j].second);
        ++count;
      }
    }

    return static_cast<double>(sum) / static_cast<double>(count);
  }

  static double get_population_score(
      const std::vector<std::pair<double, std::vector<bool>>>& population) {
    Minimum<double, std::less<>> min_score;

    for (const double s : population | std::views::keys) {
      min_score.record(s);
    }

    return *min_score;
  }

  std::pair<size_t, size_t> choose_parents(
      const std::vector<std::pair<double, std::vector<bool>>>& population) {
    size_t parent1 = 0;

    for (size_t i = 0; i + 1 < population.size(); ++i) {
      if (rnd::bernoulli(0.5, random_)) {
        parent1 = i;
        break;
      }
    }

    const size_t parent2 =
        parent1 + rnd::index(population.size() - parent1 - 1, random_) + 1;

    return {parent1, parent2};
  }

 public:
  explicit AngryCustomers(const Problem& problem, timing::Deadline deadline,
                          GeneticsParameters params)
      : problem(problem),
        deadline(deadline),
        params(params),
        improver_(problem) {}

  Solution solve() {
    // Each individual is a choice of opened facilities
    std::vector<std::pair<double, std::vector<bool>>> population;

    for (size_t i = 0; i < params.population_size; ++i) {
      auto individual = get_initial_individual();

      auto [grown_individual, solution] = grow(std::move(individual));

      population.emplace_back(get_score(problem, solution),
                              std::move(grown_individual));
    }

    size_t iteration = 0;

    while (!deadline.is_over()) {
      ++iteration;

      std::ranges::sort(population, {}, [](const auto& p) { return p.first; });

      if (iteration % 100 == 0) {
        // O, throw away the worser part of it,
        // And live the purer with the other half!
        // - Hamlet

        auto range = std::ranges::unique(population);
        // std::println("  removed: {}", range.size());

        // replace the worst half of the population with random guys
        // for (size_t i = 0; i < population.size(); ++i) {
        //   for (bool v : population[i].second) {
        //     std::print("{}", v ? 1 : 0);
        //   }
        //
        //   std::println(" - {}", population[i].first);
        // }

        // std::println("  replaced!");

        const double replaced_ratio = 0.25;

        size_t remove_start = std::min(
            static_cast<size_t>(population.size() * (1 - replaced_ratio)),
            population.size() - range.size());

        for (size_t i = remove_start; i < population.size(); ++i) {
          auto [new_individual, solution] = grow(get_initial_individual());

          population[i] = {get_score(problem, solution),
                           std::move(new_individual)};
        }
      }

      // choose random parents
      const auto [p1, p2] = choose_parents(population);

      // construct child
      auto child = crossover(population[p1].second, population[p2].second);

      if (rnd::bernoulli(params.mutation_rate, random_)) {
        child = mutation(std::move(child));
      }

      auto [grown_child, solution] = grow(std::move(child));

      const double child_score = get_score(problem, solution);

      // replacement scheme
      std::pair replacement = {child_score, std::move(grown_child)};
      const size_t replaced = find_replaced(replacement, population);

      population[replaced] = std::move(replacement);

      if (iteration % 100 == 0) {
        std::println("score = {}, diversity = {}",
                     get_population_score(population),
                     get_population_diversity(population));
      }
    }

    std::println("  total iterations = {}", iteration);

    // return the best from population
    ArgMinimum<double> best;
    for (size_t i = 0; i < population.size(); ++i) {
      best.record(i, population[i].first);
    }

    auto [_, solution] = grow(std::move(population[best->index].second));
    return Solution{solution};
  }
};

}  // namespace facility
