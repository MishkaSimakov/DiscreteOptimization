#include <print>

#include "Output.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "setcover/Evaluator.h"
#include "setcover/Reader.h"
#include "setcover/solvers/GRASP.h"

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

  auto duration = timing::timeit([&]() {
    solution =
        setcover::GRASP(0.1, 0.4, std::chrono::seconds{60}).solve(problem);
  });

  auto grasp_evaluation = setcover::evaluate(problem, solution);

  if (!grasp_evaluation.is_valid) {
    throw std::runtime_error(
        "Something went terribly wrong! Solution is invalid");
  }

  setcover::Statistics stats(grasp_evaluation, duration);

  setcover::Output().store(problem_name, solution, stats);
}

int main() {
  for (const auto& problem_name : kGradedProblems) {
    solve(problem_name);
  }

  return 0;
}
