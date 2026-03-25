#include <print>

#include "Output.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "knapsack/Evaluator.h"
#include "knapsack/Reader.h"
#include "knapsack/Types.h"
#include "knapsack/solvers/GRASP.h"

const std::vector<std::string> kGradedProblems = {
    "ks_30_0", "ks_50_0", "ks_200_0", "ks_400_0", "ks_1000_0", "ks_10000_0",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path(2, problem_name);
  auto problem = knapsack::read_problem(path);

  std::println("solving {}, #items = {}", path.filename().string(),
               problem.items.size());

  knapsack::Solution solution;

  auto duration = timing::timeit([&] {
    solution = knapsack::GRASP(0.01, std::chrono::seconds{60}).solve(problem);
  });

  auto grasp_evaluation = knapsack::evaluate(problem, solution);

  if (!grasp_evaluation.is_valid) {
    throw std::runtime_error(
        "Something went terribly wrong! Solution is invalid");
  }

  knapsack::Statistics stats(grasp_evaluation, duration);

  knapsack::Output().store(problem_name, solution, stats);
}

int main() {
  auto duration = timing::timeit([] {
    for (const auto& problem_name : kGradedProblems) {
      solve(problem_name);
    }
  });

  std::println("total duration: {}", duration);

  return 0;
}
