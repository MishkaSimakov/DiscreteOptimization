#pragma once

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <ranges>

#include "Types.h"
#include "helpers/Files.h"
#include "utils/String.h"

namespace facility {

class Output {
  void store_solution(const std::string& problem_name,
                      const Solution& solution) {
    auto os = files::open_creating_directories(
        files::solution_path("facility", problem_name));

    if (!os) {
      throw std::runtime_error("Failed to open output file.");
    }

    for (const size_t assigned_facility : solution.facility) {
      std::print(os, "{} ", assigned_facility);
    }
  }

  void store_statistics(const std::string& problem_name,
                        const Statistics& statistics) {
    auto os = files::open_creating_directories(
        files::statistics_path("facility", problem_name));

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

}  // namespace facility
