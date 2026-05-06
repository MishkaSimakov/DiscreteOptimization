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

Solution solve(const Problem& problem) {
  const auto initial_solution = Random(problem).solve();

  auto annealing =
      annealing::SimulatedAnnealing<Problem, ProblemState, Solution,
                                    SolutionState, annealing::GeometricCooling>(
          problem, timing::Deadline::after(60s));

  annealing.add<ChangeVehicleManager>("change_vehicle", 1);
  annealing.add<TwoOptManager>("2opt", 10);

  const double start_temperature =
      annealing.estimate_start_temperature(100'000, 0.5, initial_solution);

  return annealing.solve(initial_solution, annealing::GeometricCooling(
                                               start_temperature, 0.95, 100));
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
