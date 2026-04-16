#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

#include "Types.h"

namespace tsp {

inline Problem read_problem(const std::filesystem::path& path) {
  std::ifstream is(path);

  if (!is) {
    throw std::runtime_error("Failed to open problem file.");
  }

  size_t points_count;
  is >> points_count;

  std::vector<Point> points(points_count);

  for (size_t i = 0; i < points_count; ++i) {
    double x;
    double y;
    is >> x >> y;

    points[i].x = static_cast<size_t>(x);
    points[i].y = static_cast<size_t>(y);
  }

  return Problem{
      .points = std::move(points),
  };
}

}  // namespace tsp
