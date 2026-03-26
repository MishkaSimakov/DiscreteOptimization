#pragma once

#include <fstream>
#include <vector>

#include "Types.h"

namespace coloring {

inline Graph read_problem(const std::filesystem::path& path) {
  std::ifstream is(path);

  if (!is) {
    throw std::runtime_error("Failed to open problem file.");
  }

  size_t nodes_count;
  size_t edges_count;
  is >> nodes_count >> edges_count;

  std::vector<std::vector<size_t>> adjacency(nodes_count);
  for (size_t i = 0; i < edges_count; ++i) {
    size_t u, v;
    is >> u >> v;

    adjacency[u].push_back(v);
    adjacency[v].push_back(u);
  }

  return Graph{adjacency};
}

}  // namespace coloring
