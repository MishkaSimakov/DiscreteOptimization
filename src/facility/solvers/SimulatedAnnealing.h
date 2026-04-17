#pragma once

#include <cmath>
#include <iostream>
#include <random>

#include "Neighborhood.h"
#include "facility/Evaluator.h"
#include "facility/Types.h"
#include "helpers/Hashers.h"
#include "helpers/Random.h"
#include "helpers/Time.h"

namespace facility {

class SimulatedAnnealing {
  const Problem& problem;
  const timing::Deadline deadline;

  std::default_random_engine random_;

  // customer-customer neighborhood
  std::vector<std::vector<size_t>> neighbors_;

  // indices of opened facilities
  std::vector<size_t> opened_;

  // demands_[i] is the sum of i-th facility customers demands
  std::vector<double> demands_;

  // customers that belong to infeasible facilities
  std::vector<size_t> infeasible_customers_;

  size_t choose_customer(const Solution& solution) {
    if (rnd::bernoulli(0.5, random_) && !infeasible_customers_.empty()) {
      const size_t index = rnd::index(infeasible_customers_.size(), random_);

      return infeasible_customers_[index];
    }

    return rnd::index(problem.customers.size(), random_);
  }

  size_t choose_facility(const Solution& solution, size_t customer) {
    std::ranges::sort(opened_, {}, [&](size_t facility) {
      if (facility == solution.facility[customer]) {
        return 1e10;
      }

      return distance(problem.customers[customer].position,
                      problem.facilities[facility].position);
    });

    for (const size_t facility : opened_) {
      if (facility == solution.facility[customer]) {
        break;
      }

      if (rnd::bernoulli(0.4, random_)) {
        return facility;
      }
    }

    return opened_[0];
  }

 public:
  explicit SimulatedAnnealing(const Problem& problem, timing::Deadline deadline)
      : problem(problem),
        deadline(deadline),
        neighbors_(get_neighbors(problem, 100)) {}

  Solution solve(const Solution& initial_solution) {
    std::uniform_real_distribution<double> random_accept(0, 1);

    const auto [n, d] = problem.shape();
    const auto& customers = problem.customers;
    const auto& facilities = problem.facilities;

    constexpr double relative_start_temperature = 1e-4;
    constexpr double relative_end_temperature = 1e-11;
    constexpr double alpha = 0.99;  // temperature change rate
    constexpr size_t iterations_per_temperature = 10;

    Solution current_solution = initial_solution;
    double current_cost = get_score(problem, current_solution);

    Solution best_solution = current_solution;
    double best_cost = current_cost;

    double temperature = relative_start_temperature * current_cost;
    const double end_temperature = relative_end_temperature * current_cost;
    size_t iteration = 0;

    demands_.resize(n, 0);
    std::unordered_set<size_t> opened_set;

    for (size_t i = 0; i < d; ++i) {
      demands_[current_solution.facility[i]] += problem.customers[i].demand;
      opened_set.insert(current_solution.facility[i]);
    }

    opened_ = {opened_set.begin(), opened_set.end()};

    for (size_t i = 0; i < d; ++i) {
      const size_t f = current_solution.facility[i];

      if (demands_[f] > problem.facilities[f].capacity) {
        infeasible_customers_.push_back(i);
      }
    }

    double infeasibility_coef = 50;

    while (temperature > end_temperature) {
      // choose random customer and change his facility, customers from
      // infeasible facilities have larger probabilities of being chosen
      const size_t customer = choose_customer(current_solution);

      const size_t f0 = current_solution.facility[customer];
      const size_t f1 = choose_facility(current_solution, customer);

      // recalculate demands (f0 changed to f1)
      const double new_demand_f0 = demands_[f0] - customers[customer].demand;
      const double new_demand_f1 = demands_[f1] + customers[customer].demand;

      //
      double gain =
          distance(customers[customer].position, facilities[f0].position) -
          distance(customers[customer].position, facilities[f1].position);

      double infeasibility_gain =
          std::max(demands_[f0] - facilities[f0].capacity, 0.) -
          std::max(new_demand_f0 - facilities[f0].capacity, 0.) +
          std::max(demands_[f1] - facilities[f1].capacity, 0.) -
          std::max(new_demand_f1 - facilities[f1].capacity, 0.);

      gain += infeasibility_gain * infeasibility_coef;

      // accept using simulated annealing algorithm
      if (gain > 0 || std::exp(gain / temperature) > random_accept(random_)) {
        // changed feasibility state of some facility => update
        // infeasible_customers_
        bool changed_feasibility = false;

        if (demands_[f0] <= facilities[f0].capacity !=
                new_demand_f0 <= facilities[f0].capacity ||
            demands_[f1] <= facilities[f1].capacity !=
                new_demand_f1 <= facilities[f1].capacity) {
          changed_feasibility = true;
        }

        current_solution.facility[customer] = f1;
        demands_[f0] = new_demand_f0;
        demands_[f1] = new_demand_f1;

        if (changed_feasibility) {
          infeasible_customers_.clear();

          for (size_t i = 0; i < d; ++i) {
            const size_t f = current_solution.facility[i];

            if (demands_[f] > problem.facilities[f].capacity) {
              infeasible_customers_.push_back(i);
            }
          }
        }

        current_cost = get_score(problem, current_solution);
        double current_infeasibility =
            get_infeasibility(problem, current_solution);

        // std::println("  gain: {}, T = {}, {} ({} -> {}), feasible = {}", gain, temperature, customer, f0, f1, current_infeasibility);
        // std::println("  barrier = {}, infeasibility = {}, barrier gain = {}",
        // infeasibility_coef, current_infeasibility,
        // infeasibility_gain * infeasibility_coef);

        // std::println(
        //     "  gain = {}, current score = {}, best score = {}, infeasibility
        //     = {}, average = {}, temperature = {}", gain, current_cost,
        //     best_cost, get_infeasibility(problem, current_solution),
        //     average_gain.mean(), temperature);

        if (current_cost < best_cost && current_infeasibility == 0) {
          best_cost = current_cost;
          best_solution = current_solution;
        }
      }

      if ((iteration + 1) % iterations_per_temperature == 0) {
        temperature *= alpha;
        infeasibility_coef /= alpha;
      }
      ++iteration;

      if (deadline.is_over()) {
        break;
      }
    }

    // std::println("done");

    return best_solution;
  }
};

}  // namespace facility
