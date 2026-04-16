#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "solvers/AngryCustomers.h"
#include "solvers/Greedy.h"

using namespace std::chrono_literals;
using namespace facility;

Solution solve(const Problem& problem) {
  GeneticsParameters params{
      .population_size = 1000,
      .mutation_rate = 0.5,
      .similarity_replacement_threshold = 2,
  };

  return AngryCustomers(problem, timing::Deadline::after(60s), params).solve();
}

int main(int argc, char** argv) {
  if (argc != 2) {
    throw std::invalid_argument("Wrong number of arguments");
  }

  std::string problem_name = argv[1];

  auto path = files::problem_path("facility", problem_name);
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
