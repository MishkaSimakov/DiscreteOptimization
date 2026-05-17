#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"

#include "helpers/Files.h"
#include "helpers/Time.h"

#include "common/annealing/GeometricCooling.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "helpers/grid_search/GridSearch.h"
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
    "fl_25_2", "fl_100_1", "fl_200_7", "fl_500_7", "fl_1000_2", "fl_2000_2",
};

struct ProblemEntry {
  std::string name;
  Problem problem;

  std::vector<Solution> solutions;
};

std::vector<bool> get_initial_individual(const Problem& problem,
                                         std::default_random_engine& random) {
  const auto [n, d] = problem.shape();

  std::vector<bool> result(n);

  for (size_t i = 0; i < n; ++i) {
    result[i] = rnd::bernoulli(0.1, random);
  }

  return result;
}

Solution grow(const Problem& problem, std::vector<bool> individual) {
  const auto [n, d] = problem.shape();

  std::vector<size_t> result(d, 0);
  std::vector<double> current_demand(n, 0);

  std::vector<size_t> opened;
  for (size_t i = 0; i < n; ++i) {
    if (individual[i]) {
      opened.push_back(i);
    }
  }

  // choose the closest facility for each customer
  for (size_t i = 0; i < d; ++i) {
    ArgMinimum<double> closest_feasible;

    for (size_t j : opened) {
      if (current_demand[j] + problem.customers[i].demand <=
          problem.facilities[j].capacity) {
        closest_feasible.record(
            j, geom::distance_sqr(problem.customers[i].position,
                                  problem.facilities[j].position));
      }
    }

    if (closest_feasible.has_value()) {
      result[i] = closest_feasible->index;
      current_demand[result[i]] += problem.customers[i].demand;
      continue;
    }

    ArgMinimum<double> closest;

    for (const size_t j : opened) {
      closest.record(j, geom::distance_sqr(problem.customers[i].position,
                                           problem.facilities[j].position));
    }

    result[i] = closest.has_value() ? closest->index : 0;
    current_demand[result[i]] += problem.customers[i].demand;
  }

  return Solution{std::move(result)};
}

class Runner {
  std::vector<std::shared_ptr<ProblemEntry>> problems_;

  std::vector<std::shared_ptr<annealing::SimulatedAnnealing<
      Problem, ProblemState, Solution, SolutionState,
      annealing::GeometricCooling>>>
      annealings_;

 public:
  Runner(const std::vector<ProblemEntry>& problems) {
    for (const auto& entry : problems) {
      problems_.emplace_back(std::make_shared<ProblemEntry>(entry));

      constexpr annealing::SimulatedAnnealingConfig annealing_config{
          .log_best = false,
          .log_iteration_end = false,
          .verify_gain = false,
      };

      annealings_.push_back(std::make_shared<annealing::SimulatedAnnealing<
                                Problem, ProblemState, Solution, SolutionState,
                                annealing::GeometricCooling>>(
          problems_.back()->problem, annealing_config));

      annealings_.back()->add<ChangeCustomerFacilityManager>(
          "change_customer_facility", 90);
      annealings_.back()->add<SwapOpenedFacilityManager>("swap_opened_facility",
                                                         5);
    }
  }

  std::unordered_map<std::string, std::vector<std::pair<double, double>>>
  operator()(const gridsearch::Configuration& config) {
    std::unordered_map<std::string, std::vector<std::pair<double, double>>>
        result;

    const double start_temperature = config.at("start_temperature");
    const double end_temperature =
        start_temperature * config.at("end_temperature_ratio");
    const double penalty = config.at("infeasibility_penalty");

    for (size_t i = 0; i < problems_.size(); ++i) {
      std::vector<std::pair<double, double>> problem_results;

      for (const auto& solution : problems_[i]->solutions) {
        auto improved = annealings_[i]->solve(
            solution,
            annealing::GeometricCooling(start_temperature, end_temperature),
            penalty, 5ms);

        problem_results.emplace_back(
            get_score(problems_[i]->problem, improved),
            get_infeasibility(problems_[i]->problem, improved));
      }

      result.emplace(problems_[i]->name, std::move(problem_results));
    }

    return result;
  }
};

int main() {
  std::default_random_engine random;

  // generate 5 random solution for each problem
  std::vector<ProblemEntry> problems;

  for (const auto& name : graded_problems) {
    auto path = files::problem_path("facility", name);
    auto problem = read_problem(path);

    std::vector<Solution> solutions;

    for (size_t i = 0; i < 5; ++i) {
      solutions.push_back(
          grow(problem, get_initial_individual(problem, random)));
    }

    problems.emplace_back(name, problem, solutions);
  }

  const std::filesystem::path output_directory = "grid_search";
  std::filesystem::create_directories(output_directory);

  gridsearch::GridSearch<
      std::unordered_map<std::string, std::vector<std::pair<double, double>>>>
      search(output_directory);

  search.add_parameter("start_temperature",
                       {10, 20, 40, 80, 160, 320, 640, 1280, 2560});
  search.add_parameter("end_temperature_ratio",
                       {0.01, 0.05, 0.1, 0.15, 0.2, 0.25, 0.5, 0.75});
  search.add_parameter("infeasibility_penalty",
                       {10, 20, 40, 80, 160, 320, 640, 1280});

  search.set_strategy(gridsearch::GridSearchStrategy::SEQUENTIAL);

  search.set_runner(Runner(problems));

  search.start();
}
