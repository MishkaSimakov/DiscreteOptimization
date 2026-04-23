#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace files {

inline std::filesystem::path problem_path(std::string_view task_name,
                                          const std::string& problem_name) {
  return std::filesystem::path(PATH_TO_TASKS) / task_name / problem_name;
}

inline std::filesystem::path solution_path(std::string_view task_name,
                                           std::string_view problem_name) {
  return std::filesystem::path(PATH_TO_OUTPUT) / task_name / problem_name;
}

inline std::filesystem::path statistics_path(std::string_view task_name,
                                             std::string_view problem_name) {
  std::string filename(problem_name);
  filename += "_stats.json";

  return std::filesystem::path(PATH_TO_OUTPUT) / task_name / filename;
}

inline std::filesystem::directory_iterator problems_iterator(
    size_t task_index) {
  return std::filesystem::directory_iterator{
      std::filesystem::path(PATH_TO_TASKS) / std::to_string(task_index)};
}

inline std::ofstream open_creating_directories(
    const std::filesystem::path& path) {
  std::filesystem::create_directories(path.parent_path());

  return std::ofstream(path);
}

}  // namespace files
