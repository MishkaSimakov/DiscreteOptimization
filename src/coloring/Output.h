#pragma once

#include <filesystem>
#include <fstream>
#include <ranges>

#include "Types.h"
#include "helpers/Files.h"
#include "utils/String.h"

namespace coloring {

class Output {
 public:
  Output() = default;

  void store(const std::filesystem::path& path, const Solution& solution) {
    auto os = files::open_creating_directories(path);

    if (!os) {
      throw std::runtime_error("Failed to open output file.");
    }

    size_t colors_count = 0;
    for (size_t i : solution.colors) {
      colors_count = std::max(colors_count, i + 1);
    }

    // (colors_count, is_optimal)
    std::println(os, "{} 0", colors_count);

    std::println(
        os, "{}",
        str::join(solution.colors | std::views::transform([](size_t i) {
                    return std::to_string(i);
                  }),
                  " "));
  }
};

}  // namespace coloring
