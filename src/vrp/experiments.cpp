#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "solvers/Greedy.h"

using namespace std::chrono_literals;
using namespace vrp;

const std::vector<std::string> graded_problems = {
    "vrp_16_3_1",   "vrp_26_8_1",   "vrp_51_5_1",
    "vrp_101_10_1", "vrp_200_16_1", "vrp_421_41_1",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path("vrp", problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #vehicles = {}, capacity = {}, #customers = {}",
               path.filename().string(), problem.vehicles_count,
               problem.vehicle_capacity, problem.customers.size());

  auto solution = Greedy(problem).solve();

  auto evaluation = evaluate(problem, solution);
  if (!evaluation.is_valid) {
    throw std::runtime_error("Invalid solution");
  }

  std::println("  score: {}", evaluation.score);

  Statistics stats(evaluation, 0s);
  Output().store(problem_name, solution, stats);
}

int main() {
  for (const auto& problem_name : graded_problems) {
    solve(problem_name);
  }

  return 0;
}
