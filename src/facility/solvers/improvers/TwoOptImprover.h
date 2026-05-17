#pragma once

#include <optional>
#include <vector>

#include "facility/Types.h"
#include "facility/solvers/Neighborhood.h"

namespace facility {

class TwoOptImprover {
  constexpr static size_t max_iterations = 100;
  constexpr static size_t customers_near_count = 20;
  constexpr static size_t facilities_near_count = 10;

  const Problem& problem;

  // for each customer stores his neighbors, that is k closest customers
  const std::vector<std::vector<size_t>> customers_near;

  const std::vector<std::vector<size_t>> facilities_near;

  std::vector<size_t> order_;
  std::default_random_engine random_;

  void improve_impl(std::vector<size_t>& solution, std::vector<double>& demands,
                    double penalty) {
    const auto& customers = problem.customers;
    const auto& facilities = problem.facilities;

    bool changed;
    size_t iteration = 0;

    do {
      changed = false;
      ++iteration;

      // std::ranges::shuffle(order_, random_);

      for (size_t i = 0; i < problem.customers.size(); ++i) {
        // try to swap with someone
        for (size_t j : customers_near[i]) {
          const size_t f1 = solution[i];
          const size_t f2 = solution[j];

          if (f1 == f2) {
            continue;
          }

          // try to swap customers facility:
          // customer i -> f2
          // customer j -> f1

          const double score_gain =
              distance(customers[i].position, facilities[f1].position) +
              distance(customers[j].position, facilities[f2].position) -
              distance(customers[i].position, facilities[f2].position) -
              distance(customers[j].position, facilities[f1].position);

          const double new_demand_f1 =
              demands[f1] + customers[j].demand - customers[i].demand;
          const double new_demand_f2 =
              demands[f2] - customers[j].demand + customers[i].demand;

          const double infeasibility_gain =
              std::max(demands[f1] - facilities[f1].capacity, 0.) +
              std::max(demands[f2] - facilities[f2].capacity, 0.) -
              std::max(new_demand_f1 - facilities[f1].capacity, 0.) -
              std::max(new_demand_f2 - facilities[f2].capacity, 0.);

          const double gain = score_gain + penalty * infeasibility_gain;
          if (gain < 1e-10) {
            continue;
          }

          demands[f1] = new_demand_f1;
          demands[f2] = new_demand_f2;

          std::swap(solution[i], solution[j]);

          changed = true;
        }

        // try to change facility
        // const size_t f1 = solution[i];
        // for (size_t f2 : facilities_near[i]) {
        //   if (demands[f2] < 1e-10) {
        //     // facility is closed
        //     continue;
        //   }
        //
        //   double score_gain =
        //       distance(customers[i].position, facilities[f1].position) -
        //       distance(customers[i].position, facilities[f2].position);
        //
        //   const double new_demand_f1 = demands[f1] - customers[i].demand;
        //   const double new_demand_f2 = demands[f2] + customers[i].demand;
        //
        //   if (new_demand_f1 < 1e-10) {
        //     score_gain += problem.facilities[f1].cost;
        //   }
        //
        //   const double infeasibility_gain =
        //       std::max(demands[f1] - facilities[f1].capacity, 0.) +
        //       std::max(demands[f2] - facilities[f2].capacity, 0.) -
        //       std::max(new_demand_f1 - facilities[f1].capacity, 0.) -
        //       std::max(new_demand_f2 - facilities[f2].capacity, 0.);
        //
        //   const double gain = score_gain + penalty * infeasibility_gain;
        //   if (gain < 1e-10) {
        //     continue;
        //   }
        //
        //   demands[f1] = new_demand_f1;
        //   demands[f2] = new_demand_f2;
        //
        //   solution[i] = f2;
        //
        //   changed = true;
        // }
      }
    } while (changed && iteration < max_iterations);
  }

 public:
  explicit TwoOptImprover(const Problem& problem)
      : problem(problem),
        customers_near(
            get_customer_customer_neighborhood(problem, customers_near_count)),
        facilities_near(
            get_customer_facility_neighborhood(problem, facilities_near_count)),
        order_(problem.customers.size()) {
    std::iota(order_.begin(), order_.end(), 0);
  }

  std::vector<size_t> improve(std::vector<size_t> solution,
                              std::vector<double> demands) {
    // std::println("start");

    improve_impl(solution, demands, std::pow(1.5, 15));

    // for (size_t i = 0; i <= 10; ++i) {
    //   // auto copy = improved;
    //
    //   improve_impl(improved, demands, std::pow(1.5, i));
    //
    //   // std::println("  i = {}, inf: {} -> {}, score: {} -> {}", i,
    //   //              get_infeasibility(problem, copy),
    //   //              get_infeasibility(problem, improved),
    //   //              get_score(problem, copy), get_score(problem, improved));
    //
    //   bool is_feasible = true;
    //   for (size_t j = 0; j < problem.facilities.size(); ++j) {
    //     if (demands[j] > problem.facilities[j].capacity) {
    //       is_feasible = false;
    //       break;
    //     }
    //   }
    //
    //   if (is_feasible) {
    //     break;
    //   }
    // }

    // std::println("  inf: {} -> {}, score: {} -> {}",
    //              get_infeasibility(problem, copy),
    //              get_infeasibility(problem, improved), get_score(problem,
    //              copy), get_score(problem, improved));

    return std::move(solution);
  }
};

}  // namespace facility
