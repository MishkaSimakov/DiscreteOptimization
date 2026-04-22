#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "solvers/Greedy.h"

using namespace std::chrono_literals;
using namespace vrp;

Solution solve(const Problem& problem) { return Greedy(problem).solve(); }

int main(int argc, char** argv) {
  if (argc != 2) {
    throw std::invalid_argument("Wrong number of arguments");
  }

  std::string problem_name = argv[1];

  auto path = files::problem_path("vrp", problem_name);
  auto problem = read_problem(path);

  Solution solution;

  auto duration = timing::timeit([&] { solution = solve(problem); });

  auto evaluation = evaluate(problem, solution);

  if (!evaluation.is_valid) {
    throw std::runtime_error("Solution is invalid");
  }

  Statistics stats(evaluation, duration);
  Output().store(problem_name, solution, stats);
}
