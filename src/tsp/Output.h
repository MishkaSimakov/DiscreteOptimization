#pragma once

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <ranges>

#include "Types.h"
#include "helpers/Files.h"
#include "utils/String.h"

namespace tsp {

class Output {
  void store_solution(const std::string& problem_name,
                      const Solution& solution) {
    auto os = files::open_creating_directories(
        files::solution_path("tsp", problem_name));

    if (!os) {
      throw std::runtime_error("Failed to open output file.");
    }

    size_t current = 0;
    for (size_t i = 0; i < solution.next.size(); ++i) {
      std::print(os, "{} ", current);

      current = solution.next[current];
    }
  }

  void store_statistics(const std::string& problem_name,
                        const Statistics& statistics) {
    auto os = files::open_creating_directories(
        files::statistics_path("tsp", problem_name));

    if (!os) {
      throw std::runtime_error("Failed to open output file.");
    }

    nlohmann::json json = {
        {"score", statistics.score},
        {"duration", statistics.duration.count()},
    };

    os << json.dump();
  }

 public:
  Output() = default;

  void store(const std::string& problem_name, const Solution& solution,
             const Statistics& statistics) {
    store_solution(problem_name, solution);
    store_statistics(problem_name, statistics);
  }
};

}  // namespace tsp
