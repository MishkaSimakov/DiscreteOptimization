#include <chrono>
#include <cmath>
#include <filesystem>
#include <print>
#include <ranges>
#include <string>

#include "Evaluator.h"
#include "Reader.h"
#include "Types.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "common/annealing/cooling/GeometricCooling.h"
#include "common/annealing/cooling/LinearCooling.h"
#include "helpers/Files.h"
#include "helpers/grid_search/GridSearch.h"
#include "solvers/AngryCustomers.h"
#include "solvers/Greedy.h"
#include "solvers/annealing/ChangeCustomerFacility.h"
#include "solvers/annealing/CloseFacility.h"
#include "solvers/annealing/OpenFacility.h"
#include "solvers/annealing/ProblemState.h"
#include "solvers/annealing/Scorer.h"
#include "solvers/annealing/SolutionState.h"
#include "solvers/annealing/SwapOpenedFacility.h"
#include "solvers/improvers/TwoOptImprover.h"

using namespace std::chrono_literals;
using namespace facility;

const std::vector<std::string> graded_problems = {
    "fl_25_2", "fl_100_1", "fl_200_7", "fl_500_7", "fl_1000_2", "fl_2000_2",
};

struct FacilitySolverConfig {
  double relative_start_temperature;
  double infeasibility_penalty;
  double open_facility_weight;
  double swap_facility_weight;
  double close_facility_weight;
  bool is_linear_cooling;
};

Solution run_solver(const ProblemState& problem, FacilitySolverConfig config) {
  // first run genetics
  constexpr AngryCustomersParameters genetics_config{
      .population_size = 100,
      .mutation_rate = 0.5,
      .similarity_replacement_threshold = 2,
      .log = false,
  };

  auto solutions =
      AngryCustomers<TwoOptImprover>(
          problem.problem, timing::Deadline::after(60s), genetics_config)
          .solve();

  auto solution =
      SolutionState(problem.problem, solutions[0]);  // use only the best one

  // then simulated annealing
  auto annealing =
      annealing::SimulatedAnnealing<ProblemState, SolutionState>(problem);

  annealing.add<ChangeCustomerFacilityManager>("change_customer_facility", 90);
  annealing.add<SwapOpenedFacilityManager>("swap_opened_facility",
                                           config.swap_facility_weight);
  annealing.add<OpenFacilityManager>("open_facility",
                                     config.open_facility_weight);
  annealing.add<CloseFacilityManager>("close_facility",
                                      config.close_facility_weight);

  const double start_temperature =
      config.relative_start_temperature * get_score(problem, solution);
  const double infeasibility_penalty = config.infeasibility_penalty;

  if (config.is_linear_cooling) {
    return annealing
        .solve(solution, annealing::LinearCooling(start_temperature, 1),
               infeasibility_penalty, 60s)
        .solution;
  }

  return annealing
      .solve(solution, annealing::GeometricCooling(start_temperature, 1),
             infeasibility_penalty, 60s)
      .solution;
}

std::vector<double> runner(const gridsearch::Configuration& config) {
  FacilitySolverConfig solver_config{
      .relative_start_temperature = config.at("relative_start_temperature"),
      .infeasibility_penalty = config.at("infeasibility_penalty"),
      .open_facility_weight = config.at("open_facility_weight"),
      .swap_facility_weight = config.at("swap_facility_weight"),
      .close_facility_weight = config.at("close_facility_weight"),
      .is_linear_cooling = config.at("is_linear_cooling") > 0.5,
  };

  std::vector<double> scores;

  for (const auto& problem_name : graded_problems) {
    auto path = files::problem_path("facility", problem_name);
    auto problem = read_problem(path);

    const auto solution = run_solver(ProblemState(problem), solver_config);

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
  const std::filesystem::path output_directory =
      files::output_path("grid_search/facility");

  std::filesystem::create_directories(output_directory);

  gridsearch::GridSearch<std::vector<double>> search(output_directory);

  // SA parameters
  search.add_parameter("relative_start_temperature",
                       {5 * 1e-6, 1e-5, 5 * 1e-5, 1e-4, 5 * 1e-4, 1e-3, 1e-2});
  search.add_parameter("infeasibility_penalty",
                       {50, 100, 150, 200, 250, 300, 350, 400, 450, 500});
  search.add_parameter("open_facility_weight", {0, 1, 2, 5, 10, 20, 50});
  search.add_parameter("swap_facility_weight", {0, 1, 2, 5, 10, 20, 50});
  search.add_parameter("close_facility_weight", {0, 1, 2, 5, 10, 20, 50});
  search.add_parameter("is_linear_cooling", {0, 1});

  search.set_max_threads(2);
  search.set_strategy(gridsearch::GridSearchStrategy::RANDOM);
  search.set_runner(runner);

  search.start();
}
