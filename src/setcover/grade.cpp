#include <print>

#include "helpers/Files.h"
#include "setcover/Evaluator.h"
#include "setcover/Reader.h"
#include "setcover/solvers/GRASP.h"

const std::vector<std::string> kGradedProblems = {
    "sc_157_0",  "sc_330_0",   "sc_1000_11",
    "sc_5000_1", "sc_10000_5", "sc_10000_2",
};

void solve(const std::filesystem::path& path) {
  auto problem = setcover::read_problem(path);

  std::println("solving {}, #elements = {}, #sets = {}",
               path.filename().string(), problem.elements_count,
               problem.sets.size());

  auto grasp_solution =
      setcover::GRASP(0.1, 0.4, std::chrono::seconds{30}).solve(problem);
  auto grasp_evaluation = setcover::evaluate(problem, grasp_solution);

  if (!grasp_evaluation.is_valid) {
    throw std::runtime_error(
        "Something went terribly wrong! Solution is invalid");
  }

  std::println("  grasp: {}", grasp_evaluation.score);
}

int main() {
  for (const auto& file : kGradedProblems) {
    solve(files::problem_path(1, file));
  }

  return 0;
}
