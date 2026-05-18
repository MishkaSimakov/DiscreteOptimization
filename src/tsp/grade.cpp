#include <print>

#include "Output.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "common/annealing/cooling/GeometricCooling.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "solvers/Genetics.h"
#include "solvers/Greedy.h"
#include "solvers/LocalSearch2opt.h"
#include "solvers/LocalSearch3opt.h"
#include "solvers/annealing/ProblemState.h"
#include "solvers/annealing/SolutionState.h"
#include "solvers/annealing/TwoOptAction.h"
#include "tsp/Evaluator.h"
#include "tsp/Reader.h"

using namespace std::chrono_literals;
using namespace tsp;

Solution solve(const Problem& problem) {
  const auto initial_solution = Greedy(problem).solve();

  auto annealing = annealing::SimulatedAnnealing<ProblemState, SolutionState>(
      ProblemState(problem));

  annealing.add<TwoOptActionManager>("2opt", 1);

  const double start_temperature =
      annealing.estimate_start_temperature(100'000, 0.3, SolutionState(initial_solution));

  const auto sa_solution = annealing.solve(
      SolutionState(initial_solution),
      annealing::GeometricCooling(start_temperature, 0.015 * start_temperature),
      0, 10min).get_solution();

  std::println("{} -> {}", get_score(problem, initial_solution),
               get_score(problem, sa_solution));

  return sa_solution;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    throw std::invalid_argument("Wrong number of arguments");
  }

  std::string problem_name = argv[1];

  auto path = files::problem_path("tsp", problem_name);
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
