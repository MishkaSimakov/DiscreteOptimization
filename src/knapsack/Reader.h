#pragma once

#include <fstream>
#include <vector>

#include "Types.h"

namespace knapsack {

Problem read_problem(const std::filesystem::path& path) {
  std::ifstream is(path);

  size_t items_count;
  size_t max_weight;
  is >> items_count >> max_weight;

  std::vector<Item> items(items_count);
  for (size_t i = 0; i < items_count; ++i) {
    is >> items[i].cost >> items[i].weight;
  }

  return Problem{
      .items = std::move(items),
      .max_weight = max_weight,
  };
}

}  // namespace knapsack
