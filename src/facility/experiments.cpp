#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"

#include "helpers/Files.h"
#include "helpers/Time.h"

#include "common/annealing/GeometricCooling.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "solvers/AngryCustomers.h"
#include "solvers/Genetics.h"
#include "solvers/Greedy.h"
#include "solvers/Random.h"
#include "solvers/annealing/ChangeCustomerFacility.h"
#include "solvers/annealing/OpenFacility.h"
#include "solvers/annealing/SolutionState.h"
#include "solvers/annealing/SwapOpenedFacility.h"
#include "solvers/improvers/AnnealingImprover.h"
#include "solvers/improvers/TwoOptImprover.h"

using namespace std::chrono_literals;
using namespace facility;

const std::vector<std::string> graded_problems = {
    // "fl_25_2",
    // "fl_100_1",
    // "fl_200_7",
    "fl_500_7",
    // "fl_1000_2",
    // "fl_2000_2",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path("facility", problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #facilities = {}, #customers = {}",
               path.filename().string(), problem.facilities.size(),
               problem.customers.size());
  //
  // const auto initial_solution = Greedy(problem).solve();
  // Solution solution;
  //
  constexpr annealing::SimulatedAnnealingConfig config{
      .log_best = true,
      .log_iteration_end = true,
      .verify_gain = true,
  };
  //
  // // step 1
  // {
  //   auto annealing = annealing::SimulatedAnnealing<Problem, ProblemState,
  //                                                  Solution, SolutionState,
  //                                                  annealing::GeometricCooling>(
  //       problem, config);
  //
  //   annealing.add<ChangeCustomerFacilityManager>("change_customer_facility",
  //                                                90);
  //   annealing.add<SwapOpenedFacilityManager>("swap_opened_facility", 5);
  //   annealing.add<OpenFacilityManager>("open_facility", 5);
  //
  //   constexpr double initial_temperature = 1000;
  //   constexpr double infeasibility_coef = 100;
  //
  //   solution = annealing.solve(
  //       initial_solution,
  //       annealing::GeometricCooling(initial_temperature, 0.99, 100),
  //       infeasibility_coef, timing::Deadline::after(10s));
  // }
  //
  // // step 2
  //
  // while (true) {
  //   // remove facility with the least demand
  //   std::vector<double> demands(problem.facilities.size(), 0);
  //
  //   for (size_t i = 0; i < problem.customers.size(); ++i) {
  //     demands[solution.facility[i]] += problem.customers[i].demand;
  //   }
  //
  //   ArgMinimum<double> least_demand;
  //   std::vector<size_t> opened;
  //
  //   for (size_t i = 0; i < problem.facilities.size(); ++i) {
  //     if (demands[i] > 0) {
  //       opened.push_back(i);
  //       least_demand.record(i, demands[i]);
  //     }
  //   }
  //
  //   std::println("Stage 2: opened size = {}", opened.size());
  //
  //   std::erase(opened, least_demand->index);
  //
  //   std::default_random_engine random;
  //
  //   for (size_t i = 0; i < problem.customers.size(); ++i) {
  //     if (solution.facility[i] == least_demand->index) {
  //       solution.facility[i] = opened[rnd::index(opened.size(), random)];
  //     }
  //   }
  //
  //   {
  //     auto annealing = annealing::SimulatedAnnealing<Problem, ProblemState,
  //                                                    Solution, SolutionState,
  //                                                    annealing::GeometricCooling>(
  //         problem, config);
  //
  //     annealing.add<ChangeCustomerFacilityManager>("change_customer_facility",
  //                                                  90);
  //     annealing.add<SwapOpenedFacilityManager>("swap_opened_facility", 5);
  //
  //     constexpr double initial_temperature = 1000;
  //     constexpr double infeasibility_coef = 500;
  //
  //     solution = annealing.solve(
  //         solution, annealing::GeometricCooling(initial_temperature, 0.99,
  //         100), infeasibility_coef, timing::Deadline::after(15s));
  //   }
  // }

  constexpr AngryCustomersParameters params{
      .population_size = 100,
      .mutation_rate = 0.5,
      .similarity_replacement_threshold = 2,
  };

  auto solutions = AngryCustomers<TwoOptImprover>(
                       problem, timing::Deadline::after(120s), params)
                       .solve();

  auto annealing =
      annealing::SimulatedAnnealing<Problem, ProblemState, Solution,
                                    SolutionState, annealing::GeometricCooling>(
          problem, config);

  annealing.add<ChangeCustomerFacilityManager>("change_customer_facility", 90);
  annealing.add<SwapOpenedFacilityManager>("swap_opened_facility", 5);

  const double initial_temperature = 1000;
  const double infeasibility_coef = 100;

  auto solution = annealing.solve(
      solutions[0], annealing::GeometricCooling(initial_temperature, 100),
      infeasibility_coef, 60s);

  // // try closing one facility
  // // step 2
  //
  // // remove facility with the least demand
  // std::vector<double> demands(problem.facilities.size(), 0);
  //
  // for (size_t i = 0; i < problem.customers.size(); ++i) {
  //   demands[solution.facility[i]] += problem.customers[i].demand;
  // }
  //
  // ArgMinimum<double> least_demand;
  // std::vector<size_t> opened;
  //
  // for (size_t i = 0; i < problem.facilities.size(); ++i) {
  //   if (demands[i] > 0) {
  //     opened.push_back(i);
  //     least_demand.record(i, demands[i]);
  //   }
  // }
  //
  // std::println("Stage 2: opened size = {}", opened.size());
  //
  // std::erase(opened, least_demand->index);
  //
  // std::default_random_engine random;
  // for (size_t i = 0; i < problem.customers.size(); ++i) {
  //   if (solution.facility[i] == least_demand->index) {
  //     solution.facility[i] = opened[rnd::index(opened.size(), random)];
  //   }
  // }
  //
  // {
  //   auto annealing = annealing::SimulatedAnnealing<Problem, ProblemState,
  //                                                  Solution, SolutionState,
  //                                                  annealing::GeometricCooling>(
  //       problem, config);
  //
  //   annealing.add<ChangeCustomerFacilityManager>("change_customer_facility",
  //                                                90);
  //   annealing.add<SwapOpenedFacilityManager>("swap_opened_facility", 5);
  //
  //   constexpr double initial_temperature = 1000;
  //   constexpr double infeasibility_coef = 500;
  //
  //   solution = annealing.solve(
  //       solution, annealing::GeometricCooling(initial_temperature, 0.99,
  //       100), infeasibility_coef, timing::Deadline::after(15s));
  // }

  auto evaluation = evaluate(problem, solution);
  if (!evaluation.is_valid) {
    throw std::runtime_error("Invalid solution");
  }

  std::println("  score: {}", evaluation.score);

  Statistics stats(evaluation, 0s);
  Output().store(problem_name, solution, stats);
}

int main() {
  for (const auto& problem_name : graded_problems) {
    solve(problem_name);
  }

  return 0;
}
