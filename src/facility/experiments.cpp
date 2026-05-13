#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"

#include "helpers/Files.h"
#include "helpers/Time.h"

#include "common/annealing/GeometricCooling.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "solvers/AngryCustomers.h"
#include "solvers/annealing/ChangeCustomerFacility.h"
#include "solvers/annealing/OpenFacility.h"
#include "solvers/annealing/SolutionState.h"
#include "solvers/improvers/TwoOptImprover.h"

using namespace std::chrono_literals;
using namespace facility;

const std::vector<std::string> graded_problems = {
    // "fl_25_2",
    "fl_100_1", "fl_200_7", "fl_500_7", "fl_1000_2", "fl_2000_2",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path("facility", problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #facilities = {}, #customers = {}",
               path.filename().string(), problem.facilities.size(),
               problem.customers.size());

  GeneticsParameters params{
      .population_size = 100,
      .mutation_rate = 0.5,
      .similarity_replacement_threshold = 2,
  };

  const auto solution = AngryCustomers<TwoOptImprover>(
                            problem, timing::Deadline::after(60s), params)
                            .solve();

  // std::println("finished genetics, starting SA...");
  //
  // constexpr annealing::SimulatedAnnealingConfig log_config{
  //     .log_best = true,
  //     .log_iteration_end = true,
  //     .verify_gain = true,
  // };
  //
  // auto solver =
  //     annealing::SimulatedAnnealing<Problem, ProblemState, Solution,
  //                                   SolutionState,
  //                                   annealing::GeometricCooling>(
  //         problem, log_config);
  //
  // solver.add<ChangeCustomerFacilityManager>("change_customer_facility", 1);
  // solver.add<OpenFacilityManager>("open_facility", 1);
  //
  // // solver.add<SwapOpenedFacilityManager>("swap_opened_facility", 0.1);
  // // solver.add<CloseFacilityManager>("close_facility", open_close_prob / 2);
  //
  // const double start_temperature =
  //     solver.estimate_start_temperature(100'000, 0.1, solution);
  // const double infeasibility_penalty = 0.01 * start_temperature;
  //
  // solution = solver.solve(
  //     solution, annealing::GeometricCooling(start_temperature, 0.95, 100),
  //     infeasibility_penalty, timing::Deadline::after(1min));

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
