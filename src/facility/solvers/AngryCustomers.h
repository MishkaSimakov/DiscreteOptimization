#pragma once

#include <cassert>
#include <map>
#include <random>

#include "Greedy.h"
#include "Neighborhood.h"
#include "annealing/SimulatedAnnealing.h"
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

class AngryCustomers {
  const Problem& problem;
  const timing::Deadline deadline;

  const GeneticsParameters params;

  // For each customer stores the closest facility
  const std::vector<size_t> closest;

  // for each customer stores his neighbors, that is k closest customers
  const std::vector<std::vector<size_t>> neighbors;

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
                                    std::vector<double> demands,
                                    std::optional<size_t> max_iterations) {
    const auto [n, d] = problem.shape();

    const auto& customers = problem.customers;
    const auto& facilities = problem.facilities;

    bool changed;

    size_t iteration = 0;

    do {
      ++iteration;

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
        for (size_t j : neighbors[i]) {
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

          // {
          //   std::vector<size_t> order(facilities.size());
          //   std::iota(order.begin(), order.end(), 0);
          //
          //   std::ranges::sort(order, {}, [&](size_t k) {
          //     return distance(customers[i].position, facilities[k].position);
          //   });
          //
          //   size_t index = 0;
          //   for (size_t k = 0; k < facilities.size(); ++k) {
          //     if (order[k] == solution[j]) {
          //       index = k;
          //       break;
          //     }
          //   }
          //
          //   std::println("  swapped: {}, {} (closest = {}, i = {}, index =
          //   {}), gain = {}",
          //                solution[i], solution[j], closest[i], i, index,
          //                gain);
          // }
          std::swap(solution[i], solution[j]);

          changed = true;
        }
      }
    } while (changed && (!max_iterations || iteration < *max_iterations));

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

  // Returns fully grown individual and assigned facility for each customer
  std::pair<std::vector<bool>, std::vector<size_t>> grow(
      std::vector<bool> individual, std::optional<size_t> max_opt_iterations) {
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

    result =
        optimize_2opt(std::move(result), current_demand, max_opt_iterations);

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
        closest(get_closest(problem)),
        neighbors(get_neighbors(problem, 10)) {}

  Solution solve() {
    constexpr size_t max_opt_iterations = 2;

    // Each individual is a choice of opened facilities
    std::vector<std::pair<double, std::vector<bool>>> population;

    for (size_t i = 0; i < params.population_size; ++i) {
      auto individual = get_initial_individual();

      auto [grown_individual, solution] =
          grow(std::move(individual), max_opt_iterations);

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
          auto [new_individual, solution] =
              grow(get_initial_individual(), max_opt_iterations);

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

      auto [grown_child, solution] = grow(std::move(child), max_opt_iterations);

      const double child_score = get_score(problem, solution);

      // replacement scheme
      std::pair replacement = {child_score, std::move(grown_child)};
      const size_t replaced = find_replaced(replacement, population);

      population[replaced] = std::move(replacement);

      // if (iteration % 20 == 0) {
      //   std::println("score = {}, diversity = {}",
      //                get_population_score(population),
      //                get_population_diversity(population));
      // }
    }

    std::println("  total iterations = {}", iteration);

    // return the best from population
    ArgMinimum<double> best;
    for (size_t i = 0; i < population.size(); ++i) {
      best.record(i, population[i].first);
    }

    auto [_, solution] =
        grow(std::move(population[*best.argmin()].second), std::nullopt);
    return Solution{solution};
  }
};

}  // namespace facility
