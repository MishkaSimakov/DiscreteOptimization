#pragma once

#include <vector>

#include "setcover/Types.h"

namespace setcover {

class Constructive {
  struct Node {
    // уже принятые решения
    // 0 - не взяли множество, 1 - взяли
    std::vector<bool> decisions;

    // нижняя оценка на решение в поддереве этой вершины
    size_t lower_bound;
  };

  // Порядок, в котором перебираются множества.
  // sets_order_[0] - индекс множества, которое будет рассмотрено первым
  std::vector<size_t> sets_order_;

  std::vector<Node> queue_;
  std::optional<std::pair<Solution, size_t>> incumbent_;

  // Использует жадный алгоритм, чтобы найти какое-то решение, лежащее в
  // поддереве данной вершины
  static void finish_from_node(const Node& node) {

  }

 public:
  Constructive() = default;

  Solution solve(const Problem& problem) {}
};

}  // namespace setcover
