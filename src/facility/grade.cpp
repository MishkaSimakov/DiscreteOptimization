#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"
#include "common/annealing/GeometricCooling.h"
#include "common/annealing/LinearCooling.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "solvers/AngryCustomers.h"
#include "solvers/Greedy.h"
#include "solvers/annealing/ChangeCustomerFacility.h"
#include "solvers/annealing/OpenFacility.h"
#include "solvers/annealing/ProblemState.h"
#include "solvers/annealing/SolutionState.h"
#include "solvers/annealing/SwapOpenedFacility.h"
#include "solvers/improvers/TwoOptImprover.h"

using namespace std::chrono_literals;
using namespace facility;

Solution solve(const Problem& problem) {
  constexpr AngryCustomersParameters genetics_config{
      .population_size = 100,
      .mutation_rate = 0.5,
      .similarity_replacement_threshold = 2,
  };

  auto solutions = AngryCustomers<TwoOptImprover>(
                       problem, timing::Deadline::after(120s), genetics_config)
                       .solve();

  // take only the best solution
  auto solution = solutions[0];

  constexpr annealing::SimulatedAnnealingConfig annealing_config{
      .log_best = true,
      .log_iteration_end = true,
      .verify_gain = false,
  };

  auto annealing =
      annealing::SimulatedAnnealing<Problem, ProblemState, Solution,
                                    SolutionState, annealing::LinearCooling>(
          problem, annealing_config);

  annealing.add<ChangeCustomerFacilityManager>("change_customer_facility", 90);
  annealing.add<SwapOpenedFacilityManager>("swap_opened_facility", 5);
  annealing.add<OpenFacilityManager>("open_facility", 1);

  const double initial_temperature = 1e-4 * get_score(problem, solution);
  const double infeasibility_coef = 50;

  return annealing.solve(solution,
                         annealing::LinearCooling(initial_temperature, 1e-5),
                         infeasibility_coef, 120s);
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
