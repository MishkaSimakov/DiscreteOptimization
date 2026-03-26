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
    "sc_157_0",  "sc_330_0",   "sc_1000_11",
    "sc_5000_1", "sc_10000_5", "sc_10000_2",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path(1, problem_name);
  auto problem = setcover::read_problem(path);

  std::println("solving {}, #elements = {}, #sets = {}",
               path.filename().string(), problem.elements_count,
               problem.sets.size());

  setcover::Solution solution;

  auto duration = timing::timeit([&] {
    // 50s GRASP + 10s Hill Climber
    auto grasp_solution =
        setcover::GRASP(0.1, 0.4, timing::Deadline::after(50s)).solve(problem);

    solution = setcover::HillClimber(timing::Deadline::after(10s))
                   .solve(problem, grasp_solution);
  });

  auto evaluation = setcover::evaluate(problem, solution);

  if (!evaluation.is_valid) {
    throw std::runtime_error("Solution is invalid");
  }

  setcover::Statistics stats(evaluation, duration);
  setcover::Output().store(problem_name, solution, stats);
}

int main() {
  for (const auto& problem_name : kGradedProblems) {
    solve(problem_name);
  }

  return 0;
}
