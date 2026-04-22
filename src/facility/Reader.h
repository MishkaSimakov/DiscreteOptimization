#pragma once

#include <filesystem>
#include <fstream>
#include <vector>

#include "Types.h"

namespace facility {

inline Problem read_problem(const std::filesystem::path& path) {
  std::ifstream is(path);

  if (!is) {
    throw std::runtime_error("Failed to open problem file.");
  }

  size_t facilities_count;
  size_t customers_count;

  is >> facilities_count >> customers_count;

  std::vector<Facility> facilities(facilities_count);
  for (size_t i = 0; i < facilities_count; ++i) {
    is >> facilities[i].cost >> facilities[i].capacity >>
        facilities[i].position.x >> facilities[i].position.y;
  }

  std::vector<Customer> customers(customers_count);
  for (size_t i = 0; i < customers_count; ++i) {
    is >> customers[i].demand >> customers[i].position.x >>
        customers[i].position.y;
  }

  return Problem{std::move(facilities), std::move(customers)};
}

}  // namespace facility
