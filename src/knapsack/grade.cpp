#include <print>

#include "helpers/Files.h"
#include "helpers/Time.h"
#include "knapsack/Evaluator.h"
#include "knapsack/Reader.h"
#include "knapsack/Types.h"
#include "knapsack/solvers/GRASP.h"

const std::vector<std::string> kGradedProblems = {
    "ks_30_0", "ks_50_0", "ks_200_0", "ks_400_0", "ks_1000_0", "ks_10000_0",
};

void solve(const std::filesystem::path& path) {
  auto problem = knapsack::read_problem(path);

  std::println("solving {}, #items = {}", path.filename().string(),
               problem.items.size());

  auto grasp_solution =
      knapsack::GRASP(0.01, std::chrono::seconds{30}).solve(problem);
  auto grasp_evaluation = knapsack::evaluate(problem, grasp_solution);

  if (!grasp_evaluation.is_valid) {
    throw std::runtime_error(
        "Something went terribly wrong! Solution is invalid");
  }

  std::println("  grasp: {}", grasp_evaluation.score);
}

int main() {
  auto duration = timing::timeit([] {
    for (const auto& file : kGradedProblems) {
      solve(files::problem_path(2, file));
    }
  });

  std::println("total duration: {}", duration);

  return 0;
}
