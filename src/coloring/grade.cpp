#include <chrono>
#include <print>

#include "coloring/Evaluator.h"
#include "coloring/Output.h"
#include "coloring/Reader.h"
#include "coloring/solvers/Avarice.h"
#include "coloring/solvers/DSatur.h"
#include "helpers/Files.h"
#include "helpers/Time.h"

using namespace std::chrono_literals;
using namespace coloring;

Solution solve(const Graph& problem) {
  auto deadline = timing::Deadline::after(60s);

  auto dsatur_solution = DSatur().solve(problem);

  if (!evaluate(problem, dsatur_solution).is_valid) {
    throw std::runtime_error("Invalid DSatur solution!");
  }

  auto avarice = Avarice(dsatur_solution, deadline);
  return avarice.solve(problem);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    throw std::invalid_argument("Wrong number of arguments");
  }

  std::string problem_name = argv[1];

  auto path = files::problem_path(3, problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #nodes = {}", problem_name,
               problem.adjacent.size());

  Solution solution;
  auto duration = timing::timeit([&] { solution = solve(problem); });

  auto evaluation = evaluate(problem, solution);
  if (!evaluation.is_valid) {
    throw std::runtime_error("Invalid solution!");
  }

  Statistics stats(evaluation, duration);
  Output().store(problem_name, solution, stats);
}
