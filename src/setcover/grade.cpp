#include <print>

#include "Output.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "setcover/Evaluator.h"
#include "setcover/Reader.h"
#include "setcover/solvers/GRASP.h"
#include "solvers/HillClimber.h"
#include "solvers/HillClimber3.h"
#include "solvers/SimulatedAnnealing.h"

using namespace std::chrono_literals;
using namespace setcover;

Solution solve(const Problem& problem) {
  SimulatedAnnealingConfig sa_config{
      .relative_start_temperature = 1e-2,
      .relative_end_temperature = 1e-7,
      .alpha = 0.99,
      .iterations_per_temperature = 25,
      .iterations_per_move = 1,
      .taboo_duration_multiplier = 1,
  };

  GRASPConfig grasp_config{
      .temperature = 0.1,
      .quality_threshold = 0.5,
  };

  auto solutions = SimulatedAnnealing(problem, timing::Deadline::after(20s),
                                      sa_config, grasp_config)
                       .solve();

  Solution best_solution = solutions.front();

  for (const auto solution : solutions) {
    auto improved =
        HillClimber3(problem, timing::Deadline::after(10s), 3).solve(solution);

    if (get_score(problem, improved) < get_score(problem, solution)) {
      best_solution = improved;
      break;
    }
  }

  return best_solution;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    throw std::invalid_argument("Wrong number of arguments");
  }

  std::string problem_name = argv[1];

  auto path = files::problem_path("setcover", problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #elements = {}, #sets = {}",
               path.filename().string(), problem.elements_count,
               problem.sets.size());

  Solution solution;

  auto duration = timing::timeit([&] { solution = solve(problem); });

  auto evaluation = evaluate(problem, solution);

  if (!evaluation.is_valid) {
    throw std::runtime_error("Solution is invalid");
  }

  Statistics stats(evaluation, duration);
  Output().store(problem_name, solution, stats);
}
