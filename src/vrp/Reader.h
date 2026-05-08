#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

#include "Types.h"

namespace vrp {

inline Problem read_problem(const std::filesystem::path& path) {
  std::ifstream is(path);

  if (!is) {
    throw std::runtime_error("Failed to open problem file.");
  }

  size_t customers_count;
  size_t vehicles_count;
  size_t vehicle_capacity;

  is >> customers_count >> vehicles_count >> vehicle_capacity;

  // first point is the origin
  --customers_count;

  geom::Point<double> origin;
  double dummy;
  is >> dummy >> origin.x >> origin.y;

  std::vector<Customer> customers(customers_count);
  for (size_t i = 0; i < customers_count; ++i) {
    is >> customers[i].demand >> customers[i].position.x >>
        customers[i].position.y;
  }

  return Problem{
      .origin = origin,
      .vehicles_count = vehicles_count,
      .vehicle_capacity = vehicle_capacity,
      .customers = std::move(customers),
  };
}

}  // namespace vrp
