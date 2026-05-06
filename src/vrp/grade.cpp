#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"

#include "helpers/Files.h"
#include "helpers/Time.h"

#include "solvers/Random.h"

#include "common/annealing/GeometricCooling.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "solvers/annealing/ChangeVehicle.h"
#include "solvers/annealing/ProblemState.h"
#include "solvers/annealing/SolutionState.h"
#include "solvers/annealing/TwoOpt.h"

using namespace std::chrono_literals;
using namespace vrp;

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

  const double start_temperature =
      annealing.estimate_start_temperature(100'000, 0.1, initial_solution);

  const double infeasibility_penalty = start_temperature * 0.5;

  return annealing.solve(
      initial_solution,
      annealing::GeometricCooling(start_temperature, 0.99, 100),
      infeasibility_penalty);
}

Solution solve(const Problem& problem) {
  const auto initial_solution = Random(problem).solve();

  const auto slightly_better =
      run_annealing(problem, initial_solution, timing::Deadline::after(5s));

  return run_annealing(problem, slightly_better, timing::Deadline::after(290s));
}

int main(int argc, char** argv) {
  if (argc != 2) {
    throw std::invalid_argument("Wrong number of arguments");
  }

  std::string problem_name = argv[1];

  auto path = files::problem_path("vrp", problem_name);
  auto problem = read_problem(path);

  Solution solution;

  auto duration = timing::timeit([&] { solution = solve(problem); });

  auto evaluation = evaluate(problem, solution);

  if (!evaluation.is_valid) {
    throw std::runtime_error("Solution is invalid");
  }

  Statistics stats(evaluation, duration);
  Output().store(problem_name, solution, stats);
}
