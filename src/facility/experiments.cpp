#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "solvers/Greedy.h"

using namespace std::chrono_literals;
using namespace facility;

const std::vector<std::string> graded_problems = {
    "fl_25_2", "fl_100_1", "fl_200_7", "fl_500_7", "fl_1000_2", "fl_2000_2",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path("facility", problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #facilities = {}, #customers = {}",
               path.filename().string(), problem.facilities.size(),
               problem.customers.size());

  auto solution = Greedy(problem).solve();
  auto evaluation = evaluate(problem, solution);

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
