#pragma once

#include <filesystem>
#include <fstream>

namespace files {

inline std::filesystem::path problem_path(size_t task_index,
                                          const std::string& problem_name) {
  return std::filesystem::path(PATH_TO_TASKS) / std::to_string(task_index) /
         problem_name;
}

inline std::filesystem::path output_path(std::string_view task_name,
                                         std::string_view problem_name) {
  return std::filesystem::path(PATH_TO_OUTPUT) / task_name / problem_name;
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
