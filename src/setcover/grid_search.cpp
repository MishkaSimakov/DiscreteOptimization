#include <cmath>
#include <print>
#include <ranges>
#include <string>

#include "Reader.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "helpers/grid_search/GridSearch.h"
#include "solvers/GRASP.h"
#include "utils/String.h"

using namespace std::chrono_literals;

const std::vector<std::string> graded_problems = {
    "sc_157_0",  "sc_330_0",   "sc_1000_11",
    "sc_5000_1", "sc_10000_5", "sc_10000_2",
};

struct SetCoverConfiguration {
  size_t relative_start_temperature_neg_log;
  size_t relative_end_temperature_neg_log;
  size_t iterations_per_temperature;
  size_t iterations_per_move;

  std::string serialize() const {
    return std::format("{}_{}_{}_{}", relative_start_temperature_neg_log,
                       relative_end_temperature_neg_log,
                       iterations_per_temperature, iterations_per_move);
  }
};

std::vector<size_t> runner(const gridsearch::Configuration& config) {
  std::vector<size_t> scores;

  const setcover::SimulatedAnnealingConfig sa_config{
      .relative_start_temperature =
          std::pow(10, config.at("relative_start_temperature_log")),
      .relative_end_temperature =
          std::pow(10, config.at("relative_end_temperature_log")),
      .alpha = 0.99,
      .iterations_per_temperature =
          static_cast<size_t>(config.at("iterations_per_temperature")),
      .iterations_per_move =
          static_cast<size_t>(config.at("iterations_per_move")),
      .taboo_duration_multiplier = config.at("taboo_duration_multiplier"),
  };

  double temperature = config.at("grasp_temperature");
  double quality = config.at("grasp_quality");

  for (const auto& problem_name : graded_problems) {
    auto path = files::problem_path(1, problem_name);
    auto problem = setcover::read_problem(path);

    auto grasp_solution =
        setcover::GRASP(temperature, quality, timing::Deadline::after(60s),
                        sa_config)
            .solve(problem);
    auto grasp_evaluation = setcover::evaluate(problem, grasp_solution);

    if (!grasp_evaluation.is_valid) {
      throw std::runtime_error("Invalid solution");
    }

    scores.push_back(grasp_evaluation.score);
  }

  return scores;
}

int main() {
  const std::filesystem::path output_directory = "grid_search";

  std::filesystem::create_directories(output_directory);

  gridsearch::GridSearch<std::vector<size_t>> search(output_directory);

  // SA parameters
  search.add_parameter("relative_start_temperature_log", {0, -1, -2, -3, -4});
  search.add_parameter("relative_end_temperature_log", {-5, -6, -7, -8, -9});
  search.add_parameter("iterations_per_temperature",
                       {1, 2, 3, 4, 5, 10, 15, 20, 25, 30, 45, 50, 100, 200});
  search.add_parameter("iterations_per_move", {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  search.add_parameter("taboo_duration_multiplier", {0.1, 0.5, 1, 1.5, 2.});

  // GRASP parameters
  search.add_parameter("grasp_temperature",
                       {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.});
  search.add_parameter("grasp_quality",
                       {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.});

  search.set_strategy(gridsearch::GridSearchStrategy::RANDOM);
  search.set_runner(runner);

  search.start();
}
