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
#include "solvers/TabuCol.h"

const std::vector<std::string> graded_problems = {
    // "gc_50_3", "gc_70_7", "gc_100_5", "gc_250_9", "gc_500_1",
    "gc_1000_5",
};

using namespace std::chrono_literals;
using namespace coloring;

void solve(const std::filesystem::path& path) {
  auto problem = read_problem(path);
  auto problem_name = path.filename().string();

  std::println("solving {}, #nodes = {}", problem_name,
               problem.adjacent.size());

  auto dsatur_solution = DSatur().solve(problem);
  auto dsatur_evaluation = evaluate(problem, dsatur_solution);

  // if (!dsatur_evaluation.is_valid) {
  //   throw std::runtime_error("Invalid DSatur solution!");
  // }
  //
  // auto avarice_solution =
  //     Avarice(problem, timing::Deadline::after(15s)).solve(dsatur_solution);
  // auto avarice_evaluation = evaluate(problem, avarice_solution);
  //
  // if (!avarice_evaluation.is_valid) {
  //   throw std::runtime_error("Invalid Avarice solution!");
  // }

  auto tabucol_solution = dsatur_solution;
  auto deadline = timing::Deadline::after(30s);

  while (true) {
    auto score = evaluate(problem, tabucol_solution).score;
    std::println("  score: {}", score);

    auto new_solution =
        TabuCol(problem, deadline).solve(tabucol_solution, score - 1);

    if (!new_solution) {
      break;
    }

    tabucol_solution = std::move(*new_solution);
  }

  auto tabucol_evaluation = evaluate(problem, tabucol_solution);
  if (!tabucol_evaluation.is_valid) {
    throw std::runtime_error("Invalid TabuCol solution!");
  }

  std::println("  DSatur = {}, TabuCol = {}", dsatur_evaluation.score,
               tabucol_evaluation.score);
}

int main() {
  auto duration = timing::timeit([] {
    for (const auto& file : graded_problems) {
      solve(files::problem_path("coloring", file));
    }
  });

  std::println("total duration: {}", duration);

  return 0;
}
