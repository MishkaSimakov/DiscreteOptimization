#pragma once

#include <cassert>
#include <map>
#include <random>

#include "Greedy.h"
#include "facility/Types.h"
#include "helpers/Hashers.h"
#include "helpers/Random.h"
#include "utils/Accumulators.h"

namespace facility {

struct GeneticsParameters {
  size_t population_size;
  double mutation_rate;
  double similarity_replacement_threshold;
};

struct OpenedStoresHash {
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

class AngryCustomers {
  const Problem& problem;
  const timing::Deadline deadline;

  const GeneticsParameters params;

  // For each customer stores the closest facility
  const std::vector<size_t> closest;

  std::default_random_engine random_;

  static std::vector<size_t> get_closest(const Problem& problem) {
    const auto [n, d] = problem.shape();

    std::vector<size_t> closest(d);

    for (size_t i = 0; i < d; ++i) {
      ArgMinimum<double, std::less<>> dist;

      for (size_t j = 0; j < n; ++j) {
        dist.record(j, distance(problem.customers[i].position,
                                problem.facilities[j].position));
      }

      closest[i] = *dist.argmin();
    }

    return closest;
  }

  std::vector<size_t> optimize_2opt(std::vector<size_t> solution,
                                    std::vector<double> demands) {
    const auto [n, d] = problem.shape();

    const auto& customers = problem.customers;
    const auto& facilities = problem.facilities;

    bool changed;

    do {
      changed = false;

      for (size_t i = 0; i < d; ++i) {
        const double diff =
            distance(customers[i].position, facilities[solution[i]].position) -
            distance(customers[i].position, facilities[closest[i]].position);

        // if customer is already connected to the closest facility, there is
        // nothing we can do for him
        if (diff < 1e-10) {
          continue;
        }

        // otherwise, try to swap with someone
        for (size_t j = 0; j < d; ++j) {
          if (solution[i] == solution[j]) {
            continue;
          }

          // try to change j-th customer facility to assigned_facility
          if (demands[solution[i]] - customers[i].demand + customers[j].demand >
              facilities[solution[i]].capacity) {
            continue;
          }

          if (demands[solution[j]] + customers[i].demand - customers[j].demand >
              facilities[solution[j]].capacity) {
            continue;
          }

          const double gain =
              distance(customers[i].position,
                       facilities[solution[i]].position) +
              distance(customers[j].position,
                       facilities[solution[j]].position) -
              distance(customers[i].position,
                       facilities[solution[j]].position) -
              distance(customers[j].position, facilities[solution[i]].position);

          if (gain < 1e-10) {
            continue;
          }

          demands[solution[i]] += customers[j].demand - customers[i].demand;
          demands[solution[j]] += customers[i].demand - customers[j].demand;

          std::swap(solution[i], solution[j]);

          changed = true;
        }
      }
    } while (changed);

    return solution;
  }

  std::vector<bool> get_initial_individual() {
    const auto [n, d] = problem.shape();

    std::vector<bool> result(n);

    for (size_t i = 0; i < n; ++i) {
      result[i] = rnd::bernoulli(0.1, random_);
    }

    return result;
  }

  // Returns fully grown individual and assigned facility for each customer.
  // Result of this function is cached.
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

      if (min_cost.argmin().has_value()) {
        result[i] = *min_cost.argmin();
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

      assert(min_cost.argmin().has_value());

      individual[*min_cost.argmin()] = true;
      result[i] = *min_cost.argmin();
      current_demand[result[i]] += problem.customers[i].demand;
    }

    result = optimize_2opt(std::move(result), current_demand);

    return {std::move(individual), std::move(result)};
  }

  void crossover(std::vector<bool>& left, std::vector<bool>& right) {
    const auto [n, d] = problem.shape();

    for (size_t i = 0; i < n; ++i) {
      if (left[i] != right[i] && rnd::bernoulli(0.5, random_)) {
        std::swap(left[i], right[i]);
      }
    }
  }

  void mutation(std::vector<bool>& individual) {
    const size_t index = rnd::index(individual.size(), random_);
    individual[index] = !individual[index];
  }

  size_t find_replaced(
      const std::pair<double, std::vector<bool>>& replacement,
      const std::vector<std::pair<double, std::vector<bool>>>& population) {
    double best_score = 1e10;  // infinity

    ArgMinimum<double> closest;

    for (size_t i = 0; i < population.size(); ++i) {
      best_score = std::min(best_score, population[i].first);

      closest.record(i, get_distance(replacement.second, population[i].second));
    }

    if (*closest.min() < params.similarity_replacement_threshold) {
      // similarity-based replacement
      size_t closest_index = *closest.argmin();

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

    return *worst_score.argmax();
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
    Minimum<double, std::less<>> score;

    for (const double s : population | std::views::keys) {
      score.record(s);
    }

    return *score.min();
  }

 public:
  explicit AngryCustomers(const Problem& problem, timing::Deadline deadline,
                          GeneticsParameters params)
      : problem(problem),
        deadline(deadline),
        params(params),
        closest(get_closest(problem)) {
    assert(params.population_size % 2 == 0);
  }

  Solution solve() {
    // Each individual is a choice of opened facilities
    std::vector<std::pair<double, std::vector<bool>>> population;

    for (size_t i = 0; i < params.population_size; ++i) {
      auto individual = get_initial_individual();

      auto [grown_individual, solution] = grow(std::move(individual));

      population.emplace_back(get_score(problem, solution),
                              std::move(grown_individual));
    }

    std::vector<size_t> order(params.population_size);
    std::iota(order.begin(), order.end(), 0);

    size_t iteration = 0;

    while (!deadline.is_over()) {
      ++iteration;

      // keep:
      // 1. population_size / 10 of the best
      // 2. 4 * population_size / 10 random among others
      // 1 / 2 of the population in total
      // then split remaining into pairs, each pair produces 2 children
      // these 2 children are duplicated and mutated

      constexpr static double keep_best_ratio = 0.1;

      std::ranges::sort(order, {},
                        [&](size_t i) { return population[i].first; });

      std::shuffle(order.begin() + params.population_size * keep_best_ratio,
                   order.end(), random_);
      std::shuffle(order.begin(), order.begin() + params.population_size / 2,
                   random_);

      for (size_t i = 0; 2 * i < params.population_size; i += 2) {
        crossover(population[order[i]].second, population[order[i + 1]].second);

        // copy and mutate
        population[order[2 * i]] = population[order[i]];
        mutation(population[order[2 * i]].second);

        population[order[2 * i + 1]] = population[order[i + 1]];
        mutation(population[order[2 * i + 1]].second);
      }

      for (size_t i = 0; i < params.population_size; ++i) {
        auto [grown_child, solution] = grow(std::move(population[i].second));

        const double child_score = get_score(problem, solution);

        population[i] = {child_score, std::move(grown_child)};
      }

      // if (iteration % 20 == 0) {
        std::println("score = {}, diversity = {}",
                     get_population_score(population),
                     get_population_diversity(population));
      // }
    }

    std::println("total iterations: {}", iteration);

    // return the best from population
    ArgMinimum<double> best;
    for (size_t i = 0; i < population.size(); ++i) {
      best.record(i, population[i].first);
    }

    auto [_, solution] = grow(std::move(population[*best.argmin()].second));
    return Solution{solution};
  }
};

}  // namespace facility
