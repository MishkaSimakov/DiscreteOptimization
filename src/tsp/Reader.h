#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

#include "Types.h"

namespace tsp {

Problem read_problem(const std::filesystem::path& path) {
  std::ifstream is(path);

  if (!is) {
    throw std::runtime_error("Failed to open problem file.");
  }

  size_t points_count;
  is >> points_count;

  std::vector<Point> points(points_count);

  for (size_t i = 0; i < points_count; ++i) {
    is >> points[i].x >> points[i].y;
  }

  return Problem{
      .points = std::move(points),
  };
}

}  // namespace tsp
