#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"

#include "helpers/Files.h"
#include "helpers/Time.h"

#include "common/annealing/GeometricCooling.h"
#include "common/annealing/LinearCooling.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "solvers/AngryCustomers.h"
#include "solvers/Genetics.h"
#include "solvers/Greedy.h"
#include "solvers/Random.h"
#include "solvers/annealing/ChangeCustomerFacility.h"
#include "solvers/annealing/OpenFacility.h"
#include "solvers/annealing/SolutionState.h"
#include "solvers/annealing/SwapOpenedFacility.h"
#include "solvers/improvers/AnnealingImprover.h"
#include "solvers/improvers/TwoOptImprover.h"

using namespace std::chrono_literals;
using namespace facility;

const std::vector<std::string> graded_problems = {
    "fl_25_2",
    "fl_100_1",
    "fl_200_7",
    "fl_500_7",
    "fl_1000_2",
    "fl_2000_2",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path("facility", problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #facilities = {}, #customers = {}",
               path.filename().string(), problem.facilities.size(),
               problem.customers.size());
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
      .verify_gain = true,
  };

  auto annealing =
      annealing::SimulatedAnnealing<Problem, ProblemState, Solution,
                                    SolutionState, annealing::GeometricCooling>(
          problem, annealing_config);

  annealing.add<ChangeCustomerFacilityManager>("change_customer_facility", 90);
  annealing.add<SwapOpenedFacilityManager>("swap_opened_facility", 5);

  const double initial_temperature = 1e-4 * get_score(problem, solution);
  const double infeasibility_coef = 50;

  solution = annealing.solve(
      solution, annealing::LinearCooling(initial_temperature, 1e-5),
      infeasibility_coef, 60s);

  auto evaluation = evaluate(problem, solution);
  if (!evaluation.is_valid) {
    throw std::runtime_error("Invalid solution");
  }

  std::println("  score: {}", evaluation.score);

  Statistics stats(evaluation, 0s);
  Output().store(problem_name, solution, stats);
}

int main() {
  for (const auto& problem_name : graded_problems) {
    solve(problem_name);
  }

  return 0;
}
