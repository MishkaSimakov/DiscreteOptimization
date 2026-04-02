#include <print>

#include "Output.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "solvers/Genetics.h"
#include "solvers/Greedy.h"
#include "solvers/LocalSearch2opt.h"
#include "tsp/Evaluator.h"
#include "tsp/Reader.h"

using namespace std::chrono_literals;

const std::vector<std::string> graded_problems = {
    "tsp_51_1",  "tsp_100_3",  "tsp_200_2",
    "tsp_574_1", "tsp_1889_1", "tsp_33810_1",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path("tsp", problem_name);
  auto problem = tsp::read_problem(path);

  std::println("solving {}, #points = {}", path.filename().string(),
               problem.points.size());

  tsp::GeneticsParams params{
      .population_size = 10,
      .mutation_rate = 0.2,
  };

  auto solution =
      tsp::Genetics(timing::Deadline::after(10s), problem, params).solve();
  auto evaluation = tsp::evaluate(problem, solution);

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
