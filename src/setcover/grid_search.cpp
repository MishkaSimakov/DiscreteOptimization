#include <cmath>
#include <format>
#include <ranges>
#include <string>

#include "Reader.h"
#include "helpers/Files.h"
#include "helpers/GridSearch.h"
#include "helpers/Time.h"
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
  size_t removed_sets_count;
  size_t iterations_per_temperature;
  size_t iterations_per_move;

  std::string serialize() const {
    return std::format("{}_{}_{}_{}_{}", relative_start_temperature_neg_log,
                       relative_end_temperature_neg_log, removed_sets_count,
                       iterations_per_temperature, iterations_per_move);
  }
};

struct SetCoverRunResult {
  std::vector<size_t> scores;

  std::string serialize() const {
    return str::join(scores | std::views::transform([](size_t score) {
                       return std::to_string(score);
                     }),
                     ",");
  }
};

SetCoverRunResult runner(SetCoverConfiguration config) {
  std::vector<size_t> scores;

  const setcover::SimulatedAnnealingConfig sa_config{
      .relative_start_temperature =
          std::pow(0.1, config.relative_start_temperature_neg_log),
      .relative_end_temperature =
          std::pow(0.1, config.relative_end_temperature_neg_log),
      .alpha = 0.99,
      .removed_sets_count = config.removed_sets_count,
      .iterations_per_temperature = config.iterations_per_temperature,
      .iterations_per_move = config.iterations_per_move,
      .taboo_duration_multiplier = 1,
  };

  for (const auto& problem_name : graded_problems) {
    auto path = files::problem_path(1, problem_name);
    auto problem = setcover::read_problem(path);

    auto grasp_solution =
        setcover::GRASP(0.1, 0.4, timing::Deadline::after(60s), sa_config)
            .solve(problem);
    auto grasp_evaluation = setcover::evaluate(problem, grasp_solution);

    if (!grasp_evaluation.is_valid) {
      throw std::runtime_error("Invalid solution");
    }

    scores.push_back(grasp_evaluation.score);
  }

  return SetCoverRunResult{scores};
}

int main() {
  const std::filesystem::path output_directory = "grid_search";

  std::filesystem::create_directories(output_directory);

  GridSearch<SetCoverConfiguration, SetCoverRunResult> search(output_directory);

  SetCoverConfiguration current_best{
      .relative_start_temperature_neg_log = 2,
      .relative_end_temperature_neg_log = 9,
      .removed_sets_count = 4,
      .iterations_per_temperature = 5,
      .iterations_per_move = 10,
  };

  search.set_strategy(GridSearchStrategy::RANDOM);
  search.set_runner(runner);
  search.add_configurations({current_best});

  search.run();
}
