#include <print>

#include "Output.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "common/annealing/cooling/GeometricCooling.h"
#include "common/annealing/cooling/LinearCooling.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "solvers/Greedy.h"
#include "solvers/annealing/SolutionState.h"
#include "solvers/annealing/TwoOptAction.h"
#include "tsp/Evaluator.h"
#include "tsp/Reader.h"

using namespace std::chrono_literals;
using namespace tsp;

const std::vector<std::string> graded_problems = {
    // "tsp_51_1",  "tsp_100_3",  "tsp_200_2",
    // "tsp_574_1",
    "tsp_1889_1",
    "tsp_33810_1",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path("tsp", problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #points = {}", path.filename().string(),
               problem.points.size());

  const auto initial_solution = Greedy(problem).solve();

  auto annealing = annealing::SimulatedAnnealing<ProblemState, SolutionState>(
      ProblemState(problem));

  annealing.add<TwoOptActionManager>("2opt", 1);

  const double start_temperature = annealing.estimate_start_temperature(
      100'000, 0.3, SolutionState(initial_solution));

  const auto solution =
      annealing
          .solve(SolutionState(initial_solution),
                 annealing::GeometricCooling(start_temperature,
                                             0.015 * start_temperature),
                 0, 10min)
          .get_solution();

  const auto evaluation = evaluate(problem, solution);

  if (!evaluation.is_valid) {
    throw std::runtime_error("Invalid solution");
  }

  std::println("  score: {}", evaluation.score);
}

int main() {
  for (const auto& problem_name : graded_problems) {
    solve(problem_name);
  }

  return 0;
}
