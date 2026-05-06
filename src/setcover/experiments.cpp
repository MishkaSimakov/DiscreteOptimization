#include <print>

#include "Output.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "setcover/Evaluator.h"
#include "setcover/Reader.h"
#include "setcover/solvers/GRASP.h"
#include "solvers/HillClimber.h"
#include "solvers/HillClimber3.h"
#include "solvers/Probability.h"
#include "solvers/SimulatedAnnealing.h"

using namespace std::chrono_literals;
using namespace setcover;

const std::vector<std::string> graded_problems = {
    // "sc_157_0",
    // "sc_330_0",
    // "sc_1000_11",
    // "sc_5000_1",
    "sc_10000_5",
    // "sc_10000_2",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path("setcover", problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #elements = {}, #sets = {}",
               path.filename().string(), problem.elements_count,
               problem.sets.size());

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

  auto evaluation = evaluate(problem, best_solution);

  if (!evaluation.is_valid) {
    throw std::runtime_error("Simulated Annealing solution is invalid");
  }

  std::println("  Simulated Annealing = {}", evaluation.score);
}

int main() {
  for (const auto& problem_name : graded_problems) {
    solve(problem_name);
  }

  return 0;
}
