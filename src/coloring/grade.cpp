#include <iostream>

#include "coloring/Evaluator.h"
#include "coloring/Output.h"
#include "coloring/Reader.h"
#include "coloring/minizinc/CliqueFinder.h"
#include "coloring/minizinc/Generator.h"
#include "coloring/preprocessing/ConnectedComponents.h"
#include "coloring/preprocessing/NeighborhoodInclusion.h"
#include "coloring/preprocessing/SeparatingCliques.h"
#include "coloring/solvers/Avarice.h"
#include "coloring/solvers/DSatur.h"
#include "coloring/solvers/Greedy.h"
#include "helpers/Files.h"
#include "helpers/Time.h"

const std::vector<std::string> kGradedProblems = {
    "gc_50_3", "gc_70_7", "gc_100_5", "gc_250_9", "gc_500_1", "gc_1000_5",
};

using namespace std::chrono_literals;

void solve(const std::string& problem_name) {
  auto path = files::problem_path(3, problem_name);
  auto problem = coloring::read_problem(path);

  std::println("solving {}, #nodes = {}", problem_name,
               problem.adjacent.size());

  coloring::Solution solution;

  auto duration = timing::timeit([&]() {
    auto dsatur_solution = coloring::DSatur().solve(problem);

    if (!coloring::evaluate(problem, dsatur_solution).is_valid) {
      throw std::runtime_error("Invalid solution!");
    }

    auto avarice =
        coloring::Avarice(dsatur_solution, timing::Deadline::after(60s));
    solution = avarice.solve(problem);
  });

  auto evaluation = coloring::evaluate(problem, solution);
  if (!evaluation.is_valid) {
    throw std::runtime_error("Invalid solution!");
  }

  coloring::Statistics stats(evaluation, duration);

  coloring::Output().store(problem_name, solution, stats);
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
