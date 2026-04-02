#pragma once

#include <random>

#include "LocalSearch2opt.h"
#include "helpers/Random.h"
#include "helpers/Time.h"
#include "tsp/Types.h"
#include "utils/Accumulators.h"
#include "utils/Logging.h"

namespace tsp {

struct GeneticsParams {
  size_t population_size;
  double mutation_rate;

  bool log;
};

class Genetics {
  const timing::Deadline deadline;
  const Problem& problem;

  const GeneticsParams params;

  const std::vector<std::vector<size_t>> candidates;

  LocalSearch2opt improver_;
  std::default_random_engine random_;

  Solution get_initial_individual() {
    const size_t n = problem.points.size();

    std::vector<size_t> next(n, -1);
    const size_t start = rnd::index(n, random_);
    size_t current = start;

    for (size_t i = 0; i < n - 1; ++i) {
      // choose closest to the current
      ArgMinimum<double> closest;

      // first go through candidates
      for (const size_t j : candidates[current]) {
        if (j == current || next[j] != -1) {
          continue;
        }

        closest.record(j, distance(problem.points[current], problem.points[j]));
      }

      if (closest.argmin()) {
        next[current] = *closest.argmin();
        current = next[current];

        continue;
      }

      // if failed to find node among candidates, perform full search
      for (size_t j = 0; j < n; ++j) {
        if (j == current || next[j] != -1) {
          continue;
        }

        closest.record(j, distance(problem.points[current], problem.points[j]));
      }

      assert(closest.argmin().has_value());
      next[current] = *closest.argmin();
      current = next[current];
    }

    next[current] = start;

    return Solution{next};
  }

  static void reverse(std::vector<size_t>& next, const size_t from,
                      const size_t to) {
    if (from == to) {
      return;
    }

    size_t current = next[from];
    size_t prev = from;

    do {
      const size_t next_node = next[current];

      next[current] = prev;

      prev = current;
      current = next_node;
    } while (prev != to);
  }

  Solution crossover(const Solution& left, const Solution& right) const {
    const size_t n = problem.points.size();
    std::vector<size_t> child = left.next;

    size_t current = 0;
    bool any_changes = false;

    for (size_t i = 0; i < n; ++i) {
      if (child[i] != right.next[i] && i != right.next[child[i]]) {
        current = child[i];
        any_changes = true;

        child[i] = -1;
      }
    }

    if (!any_changes) {
      // same parents => same child
      return Solution{child};
    }

    // calculate fragments borders:
    // fragments[2 * i]     - fragment start
    // fragments[2 * i + 1] - fragment end
    std::vector<size_t> fragments;
    fragments.push_back(current);

    for (size_t i = 0; i < n; ++i) {
      if (child[current] == -1) {
        fragments.push_back(current);
        fragments.push_back(left.next[current]);
      }

      current = left.next[current];
    }

    fragments.pop_back();

    // connect fragments in greedy fashion
    for (size_t i = 0; 2 * i + 2 < fragments.size(); ++i) {
      const size_t node = fragments[2 * i + 1];

      ArgMinimum<double> min_distance;
      for (size_t j = 2 * i + 2; j < fragments.size(); ++j) {
        min_distance.record(
            j, distance(problem.points[node], problem.points[fragments[j]]));
      }

      assert(min_distance.argmin().has_value());

      size_t next = *min_distance.argmin();
      if (next % 2 == 0) {
        // simple case: fragment end -> start
      } else {
        // hard case: fragment end -> end
        // should reverse fragment
        reverse(child, fragments[next - 1], fragments[next]);

        std::swap(fragments[next], fragments[next - 1]);
        --next;
      }

      child[node] = fragments[next];

      std::swap(fragments[2 * i + 2], fragments[next]);
      std::swap(fragments[2 * i + 3], fragments[next + 1]);
    }

    child[fragments.back()] = fragments.front();

    assert(evaluate(problem, Solution{child}).is_valid);

    return Solution{child};
  }

  Solution mutation(Solution solution) {
    const size_t n = problem.points.size();

    // non-sequential 4-change
    // choose 4 random edges (edges are selected by their vertices)
    // rnd::unique_indices returns them sorted in descending order
    auto ranks = rnd::unique_indices(n, 4, random_);

    std::vector<size_t> vertices(4);

    size_t current = 0;
    size_t found_cnt = 0;

    for (size_t rank = 0; rank < n; ++rank) {
      if (rank == ranks[3 - found_cnt]) {
        vertices[found_cnt] = current;
        ++found_cnt;

        if (found_cnt == 4) {
          break;
        }
      }

      current = solution.next[current];
    }

    const size_t a0 = vertices[0];
    const size_t a1 = vertices[1];
    const size_t a2 = vertices[2];
    const size_t a3 = vertices[3];

    const size_t b0 = solution.next[a0];
    const size_t b1 = solution.next[a1];
    const size_t b2 = solution.next[a2];
    const size_t b3 = solution.next[a3];

    reverse(solution.next, b1, a2);
    reverse(solution.next, b2, a3);

    solution.next[a1] = b3;
    solution.next[a0] = a2;
    solution.next[b1] = a3;
    solution.next[b2] = b0;

    assert(evaluate(problem, solution).is_valid);

    // improver
    solution = improver_.solve(solution);

    return solution;
  }

  static double get_diversity(
      const std::vector<std::pair<double, Solution>>& population) {
    size_t sum = 0;
    size_t count = 0;

    for (size_t i = 0; i < population.size(); ++i) {
      for (size_t j = i + 1; j < population.size(); ++j) {
        sum += distance(population[i].second, population[j].second);
        ++count;
      }
    }

    return static_cast<double>(sum) / static_cast<double>(count);
  }

  static double get_fitness(
      const std::vector<std::pair<double, Solution>>& population) {
    double sum = 0;

    for (const double score : population | std::views::keys) {
      sum += score;
    }

    return sum / static_cast<double>(population.size());
  }

  void log(const std::vector<std::pair<double, Solution>>& population) const {
    if (!params.log) {
      return;
    }

    auto params_encoded =
        std::format("{}_{}", params.population_size, params.mutation_rate);

    logging::log_value(get_diversity(population),
                       std::format("diversity_{}.csv", params_encoded));
    logging::log_value(get_fitness(population),
                       std::format("fitness_{}.csv", params_encoded));
  }

 public:
  Genetics(timing::Deadline deadline, const Problem& problem,
           GeneticsParams params)
      : deadline(deadline),
        problem(problem),
        params(params),
        candidates(get_candidates_by_distance(problem, 5)),
        improver_(problem, candidates) {}

  Solution solve() {
    std::vector<std::pair<double, Solution>> population;

    // auto start = std::chrono::steady_clock::now();

    for (size_t i = 0; i < params.population_size; ++i) {
      auto individual = get_initial_individual();
      auto improved = improver_.solve(individual);

      double score = get_score(problem, improved);

      population.emplace_back(score, std::move(improved));

      // std::println("  average: {}",
                   // static_cast<double>(
                       // (std::chrono::steady_clock::now() - start).count()) /
                       // (i + 1));
    }

    while (!deadline.is_over()) {
      // choose random parents
      size_t p1 = rnd::index(params.population_size, random_);
      size_t p2 = rnd::index(params.population_size - 1, random_);

      if (p2 >= p1) {
        ++p2;
      }

      // construct child
      auto child = crossover(population[p1].second, population[p2].second);
      child = improver_.solve(child);

      if (rnd::bernoulli(params.mutation_rate, random_)) {
        child = mutation(std::move(child));
      }

      const double child_score = get_score(problem, child);

      // std::println("{} + {} = {}", population[p1].first,
      // population[p2].first, child_score);

      // replace the worst individual with child
      ArgMaximum<double> worst;
      for (size_t i = 0; i < population.size(); ++i) {
        worst.record(i, population[i].first);
      }

      population[*worst.argmax()] = {child_score, std::move(child)};

      log(population);
    }

    // return the best from population
    ArgMinimum<double> best;
    for (size_t i = 0; i < population.size(); ++i) {
      best.record(i, population[i].first);
    }

    return std::move(population[*best.argmin()].second);
  }
};

}  // namespace tsp

// 2839022628