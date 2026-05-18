#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"

#include "helpers/Files.h"
#include "helpers/Time.h"

#include "solvers/Greedy.h"
#include "solvers/Random.h"

#include "common/annealing/cooling/GeometricCooling.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "solvers/annealing/ChangeVehicle.h"
#include "solvers/annealing/ProblemState.h"
#include "solvers/annealing/SolutionState.h"
#include "solvers/annealing/TwoOpt.h"

using namespace std::chrono_literals;
using namespace vrp;

const std::vector<std::string> graded_problems = {
    "vrp_16_3_1",   "vrp_26_8_1",   "vrp_51_5_1",
    "vrp_101_10_1", "vrp_200_16_1", "vrp_421_41_1",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path("vrp", problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #vehicles = {}, capacity = {}, #customers = {}",
               path.filename().string(), problem.vehicles_count,
               problem.vehicle_capacity, problem.customers.size());

  const auto initial_solution = Random(problem).solve();

  // calculate initial temperature
  const auto neighbors = get_neighbors_by_distance(
      problem.customers | std::views::transform([](const Customer customer) {
        return customer.position;
      }),
      5);

  ArithmeticMean<double> average_distance;
  for (size_t i = 0; i < problem.customers.size(); ++i) {
    for (const size_t j : neighbors[i]) {
      average_distance.record(geom::distance(problem.customers[i].position,
                                             problem.customers[j].position));
    }
  }

  const double start_temperature = 1 * *average_distance;
  std::println("  T_start = {}", start_temperature);

  constexpr annealing::SimulatedAnnealingConfig config{
      .log_best = true,
      .log_iteration_end = true,
  };

  auto annealing =
      annealing::SimulatedAnnealing<Problem, ProblemState, Solution,
                                    SolutionState, annealing::GeometricCooling>(
          problem, timing::Deadline::after(60s), config);

  annealing.add<ChangeVehicleManager>("change_vehicle", 1);
  annealing.add<TwoOptManager>("2opt", 1);

  const double infeasibility_penalty = start_temperature * 0.75;

  const auto solution =
      annealing.solve(initial_solution,
                      annealing::GeometricCooling(start_temperature, 0.98, 100),
                      infeasibility_penalty);

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
