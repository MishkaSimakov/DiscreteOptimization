#include <print>

#include "Output.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "solvers/Genetics.h"
#include "solvers/Greedy.h"
#include "solvers/LocalSearch2opt.h"
#include "tsp/Evaluator.h"
#include "tsp/Reader.h"

using namespace std::chrono_literals;
using namespace tsp;

Solution solve(const Problem& problem) {
  constexpr GeneticsParams params{
      .population_size = 50,
      .mutation_rate = 0.2,
      .log = false,
      .similarity_replacement_threshold = 5,
  };

  return Genetics<LocalSearch2opt>(timing::Deadline::after(10min), problem,
                                   params)
      .solve();
}

int main(int argc, char** argv) {
  if (argc != 2) {
    throw std::invalid_argument("Wrong number of arguments");
  }

  std::string problem_name = argv[1];

  auto path = files::problem_path("tsp", problem_name);
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
