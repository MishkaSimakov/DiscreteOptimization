#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"

#include "helpers/Files.h"
#include "helpers/Time.h"

#include "solvers/Greedy.h"
#include "solvers/Random.h"

#include "common/annealing/GeometricCooling.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "solvers/annealing/ChangeVehicle.h"
#include "solvers/annealing/ProblemState.h"
#include "solvers/annealing/SolutionState.h"
#include "solvers/annealing/TwoOpt.h"

using namespace std::chrono_literals;
using namespace vrp;

const std::vector<std::string> graded_problems = {
    // "vrp_16_3_1",
    // "vrp_26_8_1",
    // "vrp_51_5_1",
  // "vrp_101_10_1",
  "vrp_200_16_1",
    // "vrp_421_41_1",
};

Solution run_annealing(const Problem& problem, const Solution& initial_solution,
                       timing::Deadline deadline) {
  constexpr annealing::SimulatedAnnealingConfig config{
      .log_best = true,
      .log_iteration_end = true,
  };

  auto annealing =
      annealing::SimulatedAnnealing<Problem, ProblemState, Solution,
                                    SolutionState, annealing::GeometricCooling>(
          problem, deadline, config);

  annealing.add<ChangeVehicleManager>("change_vehicle", 1);
  annealing.add<TwoOptManager>("2opt", 1);

  // const double start_temperature =
  // annealing.estimate_start_temperature(100'000, 0.5, initial_solution);
  const double start_temperature = 5;

  const double infeasibility_penalty = start_temperature * 0.5;

  return annealing.solve(
      initial_solution,
      annealing::GeometricCooling(start_temperature, 0.99, 100),
      infeasibility_penalty);
}

void solve(const std::string& problem_name) {
  auto path = files::problem_path("vrp", problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #vehicles = {}, capacity = {}, #customers = {}",
               path.filename().string(), problem.vehicles_count,
               problem.vehicle_capacity, problem.customers.size());

  const auto initial_solution = Random(problem).solve();

  // const auto solution1 =
  // run_annealing(problem, initial_solution, timing::Deadline::after(10s));
  const auto solution2 =
      run_annealing(problem, initial_solution, timing::Deadline::after(60s));

  auto evaluation = evaluate(problem, solution2);
  if (!evaluation.is_valid) {
    throw std::runtime_error("Invalid solution");
  }

  std::println("  score: {}", evaluation.score);

  Statistics stats(evaluation, 0s);
  Output().store(problem_name, solution2, stats);
}

int main() {
  for (const auto& problem_name : graded_problems) {
    solve(problem_name);
  }

  return 0;
}
