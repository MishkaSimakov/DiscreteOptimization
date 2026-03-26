#include <iostream>

#include "../helpers/Files.h"
#include "../helpers/Time.h"
#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"
#include "minizinc/CliqueFinder.h"
#include "minizinc/Generator.h"
#include "preprocessing/ConnectedComponents.h"
#include "preprocessing/NeighborhoodInclusion.h"
#include "preprocessing/SeparatingCliques.h"
#include "solvers/Avarice.h"
#include "solvers/DSatur.h"
#include "solvers/Greedy.h"

const std::vector<std::string> kGradedProblems = {
    "gc_50_3", "gc_70_7", "gc_100_5", "gc_250_9", "gc_500_1", "gc_1000_5",
};

using namespace std::chrono_literals;

void solve(const std::filesystem::path& path) {
  auto problem = coloring::read_problem(path);
  auto problem_name = path.filename().string();

  std::println("solving {}, #nodes = {}", problem_name,
               problem.adjacent.size());

  auto greedy_solution = coloring::Greedy().solve(problem);
  auto greedy_evaluation = coloring::evaluate(problem, greedy_solution);

  if (!greedy_evaluation.is_valid) {
    throw std::runtime_error("Invalid solution!");
  }

  auto dsatur_solution = coloring::DSatur().solve(problem);
  auto dsatur_evaluation = coloring::evaluate(problem, dsatur_solution);

  if (!dsatur_evaluation.is_valid) {
    throw std::runtime_error("Invalid solution!");
  }

  auto avarice =
      coloring::Avarice(dsatur_solution, timing::Deadline::after(60s));
  auto avarice_solution = avarice.solve(problem);
  auto avarice_evaluation = coloring::evaluate(problem, avarice_solution);

  if (!avarice_evaluation.is_valid) {
    throw std::runtime_error("Invalid solution!");
  }

  std::println("  greedy={}, dsatur={}, avarice={}", greedy_evaluation.score,
               dsatur_evaluation.score, avarice_evaluation.score);

  coloring::Output().store(files::solution_path("coloring", problem_name),
                           avarice_solution);
}

int main() {
  auto duration = timing::timeit([] {
    for (const auto& file : kGradedProblems) {
      solve(files::problem_path(3, file));
    }
  });

  std::println("total duration: {}", duration);

  return 0;
}
