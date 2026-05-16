#pragma once

#include <cassert>
#include <random>

#include "facility/Types.h"
#include "helpers/Hashers.h"
#include "helpers/Random.h"
#include "helpers/Time.h"
#include "utils/Accumulators.h"

namespace facility {

struct AngryCustomersParameters {
  size_t population_size;
  double mutation_rate;
  double similarity_replacement_threshold;
};

template <typename Improver>
class AngryCustomers {
  const Problem& problem;
  const timing::Deadline deadline;

  const AngryCustomersParameters params;

  Improver improver_;
  std::default_random_engine random_;

  struct IndividualHasher {
    size_t operator()(const std::vector<bool>& opened) const {
      StreamHasher hasher;

      for (size_t i = 0; i < opened.size(); ++i) {
        if (opened[i]) {
          hasher << i;
        }
      }

      return hasher.get_hash();
    }
  };
  std::unordered_map<std::vector<bool>, std::vector<size_t>, IndividualHasher>
      grow_cache_;

  // (infeasibility, cost)
  using Score = std::pair<double, double>;

  Score score(const std::vector<size_t>& solution) const {
    return {get_infeasibility(problem, solution), get_score(problem, solution)};
  }

  std::vector<bool> get_initial_individual() {
    const auto [n, d] = problem.shape();

    std::vector<bool> result(n);

    for (size_t i = 0; i < n; ++i) {
      result[i] = rnd::bernoulli(0.1, random_);
    }

    return result;
  }

  // Returns assigned facility for each customer, solution may be infeasible.
  std::vector<size_t> grow(std::vector<bool> individual) {
    // auto itr = grow_cache_.find(individual);

    // if (itr != grow_cache_.end()) {
    // return itr->second;
    // }

    const auto [n, d] = problem.shape();

    std::vector<size_t> result(d, 0);
    std::vector<double> current_demand(n, 0);

    std::vector<size_t> opened;
    for (size_t i = 0; i < n; ++i) {
      if (individual[i]) {
        opened.push_back(i);
      }
    }

    // choose the closest facility for each customer
    for (size_t i = 0; i < d; ++i) {
      ArgMinimum<double> closest_feasible;

      for (size_t j : opened) {
        if (current_demand[j] + problem.customers[i].demand <=
            problem.facilities[j].capacity) {
          closest_feasible.record(
              j, geom::distance_sqr(problem.customers[i].position,
                                    problem.facilities[j].position));
        }
      }

      if (closest_feasible.has_value()) {
        result[i] = closest_feasible->index;
        current_demand[result[i]] += problem.customers[i].demand;
        continue;
      }

      ArgMinimum<double> closest;

      for (size_t j : opened) {
        closest.record(j, geom::distance_sqr(problem.customers[i].position,
                                             problem.facilities[j].position));
      }

      // if (rnd::bernoulli(0.95, random_)) {
      result[i] = closest.has_value() ? closest->index : 0;
      // } else {
      // result[i] =
      // opened.size() > 0 ? opened[rnd::index(opened.size(), random_)] : 0;
      // }
      current_demand[result[i]] += problem.customers[i].demand;
    }

    result = improver_.improve(std::move(result), std::move(current_demand));

    grow_cache_.emplace(individual, result);

    return std::move(result);
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

  std::vector<bool> mutation(std::vector<bool> individual, bool is_feasible) {
    const double closing_prob = 0.05;
    const double opening_prob = is_feasible ? 0.04 : 0.1;

    for (size_t i = 0; i < individual.size(); ++i) {
      if (individual[i] && rnd::bernoulli(closing_prob, random_)) {
        individual[i] = false;
      } else if (!individual[i] && rnd::bernoulli(opening_prob, random_)) {
        individual[i] = true;
      }
    }

    // size_t index = rnd::index(individual.size(), random_);
    // individual[index] = !individual[index];

    return individual;
  }

  size_t find_replaced(
      const std::pair<Score, std::vector<bool>>& replacement,
      const std::vector<std::pair<Score, std::vector<bool>>>& population) {
    Minimum<Score> best_score;
    ArgMinimum<double> min_distance;

    for (size_t i = 0; i < population.size(); ++i) {
      best_score.record(population[i].first);

      min_distance.record(
          i, get_distance(replacement.second, population[i].second));
    }

    if (min_distance->min < params.similarity_replacement_threshold) {
      // similarity-based replacement
      if (replacement.first < population[min_distance->index].first) {
        return min_distance->index;
      }
    }

    // return the worst one
    ArgMaximum<Score> worst_score;

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
      const std::vector<std::pair<Score, std::vector<bool>>>& population) {
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

  static Score get_population_score(
      const std::vector<std::pair<Score, std::vector<bool>>>& population) {
    Minimum<Score> min_score;

    for (const auto s : population | std::views::keys) {
      min_score.record(s);
    }

    return *min_score;
  }

  std::pair<size_t, size_t> choose_parents(
      const std::vector<std::pair<Score, std::vector<bool>>>& population) {
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
                          AngryCustomersParameters params)
      : problem(problem),
        deadline(deadline),
        params(params),
        improver_(problem) {}

  Solution solve() {
    // Each individual is a choice of opened facilities
    std::vector<std::pair<Score, std::vector<bool>>> population;

    for (size_t i = 0; i < params.population_size; ++i) {
      auto individual = get_initial_individual();

      population.emplace_back(score(grow(individual)), std::move(individual));
    }

    size_t iteration = 0;

    while (!deadline.is_over()) {
      ++iteration;

      std::ranges::sort(population, {}, [](const auto& p) { return p.first; });

      const bool is_feasible = population[0].first.first == 0;

      if (iteration % 100 == 0) {
        // O, throw away the worser part of it,
        // And live the purer with the other half!
        // - Hamlet

        auto range = std::ranges::unique(population);

        // replace the worst half of the population with random guys

        const double replaced_ratio = 0.25;

        const size_t remove_start = std::min(
            static_cast<size_t>(population.size() * (1 - replaced_ratio)),
            population.size() - range.size());

        for (size_t i = remove_start; i < population.size(); ++i) {
          auto individual = get_initial_individual();

          population[i] = {score(grow(individual)), std::move(individual)};
        }

        std::ranges::sort(population, {},
                          [](const auto& p) { return p.first; });

        // log results
        // for (size_t i = 0; i < population.size(); ++i) {
        //   for (bool v : population[i].second) {
        //     std::print("{}", v ? 1 : 0);
        //   }
        //
        //   std::println(" - {}", population[i].first);
        // }
      }

      // choose random parents
      const auto [p1, p2] = choose_parents(population);

      // construct child
      auto child = crossover(population[p1].second, population[p2].second);

      if (rnd::bernoulli(params.mutation_rate, random_)) {
        child = mutation(std::move(child), is_feasible);
      }

      // replacement scheme
      std::pair replacement = {score(grow(child)), std::move(child)};
      const size_t replaced = find_replaced(replacement, population);

      population[replaced] = std::move(replacement);

      if (iteration % 100 == 0) {
        const auto score = get_population_score(population);

        std::println("score = ({}, {}), diversity = {}", score.first,
                     score.second, get_population_diversity(population));
      }
    }

    std::println("  total iterations = {}", iteration);

    // return the best from population
    ArgMinimum<Score> best;
    for (size_t i = 0; i < population.size(); ++i) {
      best.record(i, population[i].first);
    }

    return Solution{grow(std::move(population[best->index].second))};
  }
};

}  // namespace facility
