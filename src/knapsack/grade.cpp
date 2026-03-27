#include <print>

#include "Output.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "knapsack/Evaluator.h"
#include "knapsack/Reader.h"
#include "knapsack/Types.h"
#include "knapsack/solvers/GRASP.h"

using namespace std::chrono_literals;
using namespace knapsack;

Solution solve(const Problem& problem) {
  return GRASP(0.5, timing::Deadline::after(60s)).solve(problem);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    throw std::invalid_argument("Wrong number of arguments");
  }

  std::string problem_name = argv[1];

  auto path = files::problem_path(2, problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #items = {}", problem_name, problem.items.size());

  Solution solution;
  auto duration = timing::timeit([&] { solution = solve(problem); });

  auto evaluation = evaluate(problem, solution);

  if (!evaluation.is_valid) {
    throw std::runtime_error("Solution is invalid");
  }

  Statistics stats(evaluation, duration);
  Output().store(problem_name, solution, stats);
}
