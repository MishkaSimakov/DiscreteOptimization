#pragma once

#include <random>

#include "LocalSearch2opt.h"
#include "helpers/Random.h"
#include "helpers/Time.h"
#include "tsp/Types.h"
#include "utils/Accumulators.h"

namespace tsp {

struct GeneticsParams {
  size_t population_size;
  double mutation_rate;
};

class Genetics {
  const timing::Deadline deadline;
  const Problem& problem;

  const GeneticsParams params;

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

      for (size_t j = 0; j < n; ++j) {
        if (j == current || next[j] != -1) {
          continue;
        }

        closest.record(j, distance(problem.points[current], problem.points[j]));
      }

      next[current] = *closest.argmin();
      current = next[current];
    }

    next[current] = start;

    return Solution{next};
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
        for (size_t j = fragments[next - 1]; j != fragments[next];
             j = left.next[j]) {
          child[left.next[j]] = j;
        }

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
    // non-sequential 4-change
    // improver

    return solution;
  }

 public:
  Genetics(timing::Deadline deadline, const Problem& problem,
           GeneticsParams params)
      : deadline(deadline),
        problem(problem),
        params(params),
        improver_(problem) {}

  Solution solve() {
    const size_t n = problem.points.size();
    std::vector<std::pair<double, Solution>> population;

    for (size_t i = 0; i < params.population_size; ++i) {
      auto individual = get_initial_individual();
      auto improved = improver_.solve(individual);

      double score = get_score(problem, improved);

      population.emplace_back(score, std::move(improved));
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

      // std::println("{} + {} = {}", population[p1].first, population[p2].first, child_score);

      // replace the worst individual with child
      ArgMaximum<double> worst;
      for (size_t i = 0; i < population.size(); ++i) {
        worst.record(i, population[i].first);
      }

      population[*worst.argmax()] = {child_score, std::move(child)};
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
