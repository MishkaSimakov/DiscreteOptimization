#pragma once

#include <random>
#include <ranges>

#include "Neighborhood.h"
#include "facility/Evaluator.h"
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
class Genetics {
  constexpr static size_t max_iterations = 100;
  constexpr static size_t neighborhood_size = 20;

  const Problem& problem;
  const timing::Deadline deadline;

  const GeneticsParameters params;

  // for each customer stores his neighbors, that is k closest facilities
  const std::vector<std::vector<size_t>> neighbors;

  Improver improver_;

  std::default_random_engine random_;

  // (infeasibility, cost)
  using Score = std::pair<double, double>;
  using Individual = std::vector<size_t>;

  Score score(const Individual& solution) const {
    return {get_infeasibility(problem, solution), get_score(problem, solution)};
  }

  Individual get_initial_individual() {
    const auto [n, d] = problem.shape();

    std::vector<size_t> result(d);

    for (size_t i = 0; i < d; ++i) {
      result[i] = neighbors[i][rnd::index(neighbors[i].size(), random_)];
    }

    return result;
  }

  Individual crossover(const Individual& left, const Individual& right) {
    const auto [n, d] = problem.shape();
    std::vector<size_t> result(d);

    for (size_t i = 0; i < d; ++i) {
      result[i] = rnd::bernoulli(0.5, random_) ? left[i] : right[i];
    }

    return result;
  }

  Individual mutation(Individual individual) {
    for (size_t i = 0; i < individual.size(); ++i) {
      if (rnd::bernoulli(0.05, random_)) {
        individual[i] = neighbors[i][rnd::index(neighbors[i].size(), random_)];
      }
    }

    return individual;
  }

  Individual grow(Individual individual) {
    std::vector<double> demands(problem.facilities.size(), 0);

    for (size_t i = 0; i < problem.customers.size(); ++i) {
      demands[individual[i]] += problem.customers[i].demand;
    }

    return improver_.improve(std::move(individual), std::move(demands));
  }

  size_t find_replaced(
      const std::pair<Score, Individual>& replacement,
      const std::vector<std::pair<Score, Individual>>& population) {
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

  static size_t get_distance(const Individual& left, const Individual& right) {
    size_t result = 0;

    for (size_t i = 0; i < left.size(); ++i) {
      if (left[i] != right[i]) {
        ++result;
      }
    }

    return result;
  }

  static double get_population_diversity(
      const std::vector<std::pair<Score, Individual>>& population) {
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
      const std::vector<std::pair<Score, Individual>>& population) {
    Minimum<Score> min_score;

    for (const auto s : population | std::views::keys) {
      min_score.record(s);
    }

    return *min_score;
  }

  std::pair<size_t, size_t> choose_parents(
      const std::vector<std::pair<Score, Individual>>& population) {
    size_t parent1 = 0;

    for (size_t i = 0; i + 1 < population.size(); ++i) {
      if (rnd::bernoulli(0.3, random_)) {
        parent1 = i;
        break;
      }
    }

    const size_t parent2 =
        parent1 + rnd::index(population.size() - parent1 - 1, random_) + 1;

    return {parent1, parent2};
  }

 public:
  explicit Genetics(const Problem& problem, timing::Deadline deadline,
                    GeneticsParameters params)
      : problem(problem),
        deadline(deadline),
        params(params),
        neighbors(
            get_customer_facility_neighborhood(problem, neighborhood_size)),
        improver_(problem) {}

  Solution solve() {
    // Each individual is a choice of facility for each customer
    std::vector<std::pair<Score, Individual>> population;

    for (size_t i = 0; i < params.population_size; ++i) {
      auto individual = get_initial_individual();
      individual = grow(std::move(individual));
      population.emplace_back(score(individual), std::move(individual));
    }

    size_t iteration = 0;

    while (!deadline.is_over()) {
      ++iteration;

      std::ranges::sort(population, {}, [](const auto& p) { return p.first; });

      if (iteration % 100 == 0) {
        auto range = std::ranges::unique(population);

        const double replaced_ratio = 0.25;

        const size_t remove_start = std::min(
            static_cast<size_t>(population.size() * (1 - replaced_ratio)),
            population.size() - range.size());

        for (size_t i = remove_start; i < population.size(); ++i) {
          auto individual = get_initial_individual();
          individual = grow(std::move(individual));
          population[i] = {score(individual), std::move(individual)};
        }

        std::ranges::sort(population, {},
                          [](const auto& p) { return p.first; });

        // log results
        // for (size_t i = 0; i < population.size(); ++i) {
        //   for (size_t v : population[i].second) {
        //     std::print("{} ", v);
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
        child = mutation(std::move(child));
      }

      // replacement scheme
      child = grow(std::move(child));
      std::pair replacement = {score(child), std::move(child)};
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
    return Solution{std::move(population[best->index].second)};
  }
};

}  // namespace facility
