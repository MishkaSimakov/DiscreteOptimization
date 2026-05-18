#include <cmath>
#include <print>
#include <ranges>
#include <string>

#include "common/annealing/cooling/GeometricCooling.h"
#include "Evaluator.h"
#include "Reader.h"
#include "common/annealing/SimulatedAnnealing.h"

#include "helpers/Files.h"
#include "helpers/Time.h"
#include "helpers/grid_search/GridSearch.h"
#include "solvers/Random.h"
#include "solvers/annealing/ChangeVehicle.h"
#include "solvers/annealing/ProblemState.h"
#include "solvers/annealing/SolutionState.h"
#include "solvers/annealing/TwoOpt.h"
#include "utils/String.h"

using namespace std::chrono_literals;
using namespace vrp;

const std::vector<std::string> graded_problems = {
    "vrp_16_3_1",   "vrp_26_8_1",   "vrp_51_5_1",
    "vrp_101_10_1", "vrp_200_16_1", "vrp_421_41_1",
};

struct FacilitySolverConfig {
  double init_acceptance_rate;
  double infeasibility_penalty;
  double change_vehicle_weight;
  double alpha;
};

Solution run_annealing(const Problem& problem, const Solution& initial_solution,
                       timing::Deadline deadline,
                       FacilitySolverConfig solver_config) {
  constexpr annealing::SimulatedAnnealingConfig log_settings{
      .log_best = false,
      .log_iteration_end = false,
  };

  auto annealing =
      annealing::SimulatedAnnealing<Problem, ProblemState, Solution,
                                    SolutionState, annealing::GeometricCooling>(
          problem, deadline, log_settings);

  annealing.add<ChangeVehicleManager>("change_vehicle",
                                      solver_config.change_vehicle_weight);
  annealing.add<TwoOptManager>("2opt", 1);

  const double start_temperature = annealing.estimate_start_temperature(
      100'000, solver_config.init_acceptance_rate, initial_solution);

  const double infeasibility_penalty =
      start_temperature * solver_config.infeasibility_penalty;

  return annealing.solve(
      initial_solution,
      annealing::GeometricCooling(start_temperature, solver_config.alpha, 100),
      infeasibility_penalty);
}

std::vector<double> runner(const gridsearch::Configuration& config) {
  FacilitySolverConfig solver_config{
      .init_acceptance_rate = config.at("init_acceptance_rate"),
      .infeasibility_penalty = config.at("infeasibility_penalty"),
      .change_vehicle_weight = config.at("change_vehicle_weight"),
      .alpha = config.at("alpha"),
  };

  std::vector<double> scores;

  for (const auto& problem_name : graded_problems) {
    auto path = files::problem_path("vrp", problem_name);
    auto problem = read_problem(path);

    const auto initial_solution = Random(problem).solve();

    const auto slightly_better = run_annealing(
        problem, initial_solution, timing::Deadline::after(10s), solver_config);
    const auto solution = run_annealing(
        problem, slightly_better, timing::Deadline::after(60s), solver_config);

    const auto evaluation = evaluate(problem, solution);

    if (!evaluation.is_valid) {
      scores.push_back(1e9);
    } else {
      scores.push_back(evaluation.score);
    }
  }

  return scores;
}

int main() {
  const std::filesystem::path output_directory = "grid_search";

  std::filesystem::create_directories(output_directory);

  gridsearch::GridSearch<std::vector<double>> search(output_directory);

  // SA parameters
  search.add_parameter("init_acceptance_rate",
                       {0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7});
  search.add_parameter("infeasibility_penalty",
                       {0.25, 0.5, 0.75, 1., 1.25, 1.5, 1.75, 2});
  search.add_parameter("change_vehicle_weight", {0.5, 1, 2, 4});
  search.add_parameter("alpha", {0.95, 0.96, 0.97, 0.98, 0.99});

  search.set_strategy(gridsearch::GridSearchStrategy::RANDOM);
  search.set_runner(runner);

  search.start();
}
