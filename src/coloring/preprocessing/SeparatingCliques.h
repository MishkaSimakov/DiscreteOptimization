#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <unordered_set>

#include "coloring/Types.h"

namespace coloring {

class SeparatingCliques {
  std::tuple<Graph, std::vector<size_t>, std::vector<size_t>> lex_m(
      const Graph& graph) {
    size_t n = graph.adjacent.size();

    std::vector<std::vector<size_t>> elimination_graph(n);

    std::vector<size_t> labels(n, 0);
    std::vector<size_t> ordering(n, n);
    std::vector<size_t> ordering_inv(n, n);

    std::vector<bool> is_reached(n, false);
    std::vector<std::unordered_set<size_t>> reach;

    std::map<size_t, size_t> labels_map;

    size_t k = 0;

    for (size_t i = 0; i < n; ++i) {
      reach.resize(k + 1);

      size_t v = 0;
      while (labels[v] != 2 * k || ordering_inv[v] != n) {
        ++v;
      }

      is_reached[v] = true;
      ordering[n - i - 1] = v;
      ordering_inv[v] = n - i - 1;

      for (size_t j = 0; j <= k; ++j) {
        reach[j].clear();
      }

      for (size_t j = 0; j < n; ++j) {
        if (ordering_inv[j] == n) {
          is_reached[j] = false;
        }
      }

      for (size_t w : graph.adjacent[v]) {
        if (ordering_inv[w] == n) {
          reach[labels[w] / 2].insert(w);
          is_reached[w] = true;

          labels[w] += 1;

          // mark (v, w) as an edge
          elimination_graph[v].push_back(w);
          elimination_graph[w].push_back(v);
        }
      }

      for (size_t j = 0; j <= k; ++j) {
        while (!reach[j].empty()) {
          size_t w = *reach[j].begin();
          reach[j].erase(reach[j].begin());

          for (size_t z : graph.adjacent[w]) {
            if (!is_reached[z]) {
              is_reached[z] = true;

              if (labels[z] / 2 > j) {
                reach[labels[z] / 2].insert(z);
                labels[z] += 1;

                // mark (v, z) as an edge
                elimination_graph[v].push_back(z);
                elimination_graph[z].push_back(v);
              } else {
                reach[j].insert(z);
              }
            }
          }
        }
      }

      // sort
      labels_map.clear();
      for (size_t j = 0; j < n; ++j) {
        if (ordering_inv[j] == n) {
          labels_map.emplace(labels[j], 0);
        }
      }

      size_t j = 0;
      for (auto& [value, index] : labels_map) {
        index = 2 * j;
        ++j;
      }

      k = j - 1;

      for (size_t j = 0; j < n; ++j) {
        if (ordering_inv[j] == n) {
          labels[j] = labels_map[labels[j]];
        }
      }
    }

    return {Graph{elimination_graph}, ordering, ordering_inv};
  }

  void dfs(size_t node, const Graph& graph, std::vector<size_t>& components,
           size_t components_cnt) {
    size_t n = graph.adjacent.size();

    components[node] = components_cnt;

    for (size_t adj : graph.adjacent[node]) {
      if (components[adj] == n) {
        dfs(adj, graph, components, components_cnt);
      }
    }
  }

  // checks that C(v) is clique in G
  bool check_clique(size_t v, const Graph& problem,
                    const std::vector<size_t>& ordering_inv) {
    std::unordered_set<size_t> c;

    for (size_t adj : problem.adjacent[v]) {
      if (ordering_inv[adj] > ordering_inv[v]) {
        c.insert(adj);
      }
    }

    for (size_t i : c) {
      auto copy = c;
      copy.erase(i);

      for (size_t j : problem.adjacent[i]) {
        copy.erase(j);
      }

      if (!copy.empty()) {
        return false;
      }
    }

    return true;
  }

 public:
  SeparatingCliques() = default;

  void apply(const Graph& problem) {
    size_t n = problem.adjacent.size();
    auto [elimination_graph, ordering, ordering_inv] = lex_m(problem);

    // separating cliques are not included into components
    std::vector<size_t> components(n, n);
    size_t components_cnt = 0;

    for (size_t i : ordering) {
      // mark C(v) so that dfs would not follow paths through it
      for (size_t adj : elimination_graph.adjacent[i]) {
        if (ordering_inv[adj] > ordering_inv[i]) {
          // all clique elements must be unvisited
          assert(components[adj] == n);

          components[adj] = components_cnt;
        }
      }

      dfs(i, elimination_graph, components, components_cnt);

      bool visited_all = true;
      for (size_t j = 0; j < n; ++j) {
        if (components[j] == n) {
          visited_all = false;
          break;
        }
      }

      if (!visited_all && check_clique(i, problem, ordering_inv)) {
        // found new component
        for (size_t j = 0; j < n; ++j) {
          if (components[j] == components_cnt) {
            std::cout << j << " ";
          }
        }

        std::cout << " (";
        for (size_t adj : elimination_graph.adjacent[i]) {
          if (ordering_inv[adj] > ordering_inv[i]) {
            std::cout << adj << " ";
          }
        }

        std::cout << ")" << std::endl;

        ++components_cnt;

        for (size_t adj : elimination_graph.adjacent[i]) {
          if (ordering_inv[adj] > ordering_inv[i]) {
            components[adj] = n;
          }
        }
      } else {
        // unmark all traversed nodes
        for (size_t j = 0; j < n; ++j) {
          if (components[j] == components_cnt) {
            components[j] = n;
          }
        }
      }
    }

    // leftover is another component
    for (size_t i = 0; i < n; ++i) {
      if (components[i] == n) {
        components[i] = components_cnt;
      }
    }

    for (size_t j = 0; j < n; ++j) {
      if (components[j] == components_cnt) {
        std::cout << j << " ";
      }
    }
    std::cout << std::endl;
    ++components_cnt;
  }
};

}  // namespace coloring
