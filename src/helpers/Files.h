#include <filesystem>

namespace files {

inline std::filesystem::path problem_path(size_t task_index,
                                          const std::string& problem_name) {
  return std::filesystem::path(PATH_TO_TASKS) / std::to_string(task_index) /
         problem_name;
}

inline std::filesystem::directory_iterator problems_iterator(
    size_t task_index) {
  return std::filesystem::directory_iterator{
      std::filesystem::path(PATH_TO_TASKS) / std::to_string(task_index)};
}

}  // namespace files
