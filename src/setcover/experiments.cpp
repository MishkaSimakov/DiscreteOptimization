#include <print>

#include "Output.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "setcover/Evaluator.h"
#include "setcover/Reader.h"
#include "setcover/solvers/GRASP.h"
#include "solvers/HillClimber.h"

using namespace std::chrono_literals;

const std::vector<std::string> kGradedProblems = {
    // "sc_157_0",  "sc_330_0",   "sc_1000_11",
    // "sc_5000_1", "sc_10000_5", "sc_10000_2",

  "sc_10000_2"
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path(1, problem_name);
  auto problem = setcover::read_problem(path);

  std::println("solving {}, #elements = {}, #sets = {}",
               path.filename().string(), problem.elements_count,
               problem.sets.size());

  setcover::SimulatedAnnealingConfig sa_config{
      .relative_start_temperature = 1e-2,
      .relative_end_temperature = 1e-9,
      .alpha = 0.99,
      .iterations_per_temperature = 5,
      .iterations_per_move = 5,
      .taboo_duration_multiplier = 1,
  };

  auto grasp_solution =
      setcover::GRASP(0.1, 0.4, timing::Deadline::after(60s), sa_config)
          .solve(problem);
  auto grasp_evaluation = setcover::evaluate(problem, grasp_solution);

  if (!grasp_evaluation.is_valid) {
    throw std::runtime_error("GRASP solution is invalid");
  }

  std::println("  GRASP = {}", grasp_evaluation.score);
}

int main() {
  for (const auto& problem_name : kGradedProblems) {
    solve(problem_name);
  }

  return 0;
}
