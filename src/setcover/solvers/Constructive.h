#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <numeric>
#include <queue>
#include <random>
#include <span>
#include <vector>

#include "linear/matrix/Matrix.h"
#include "linear/model/LP.h"
#include "linear/simplex/Settings.h"
#include "linear/simplex/Simplex.h"
#include "setcover/CoveringSetsPack.h"
#include "setcover/Evaluator.h"
#include "setcover/Types.h"
#include "utils/Accumulators.h"
#include "utils/GraphvizDrawer.h"

namespace setcover {

struct Node {
  size_t id;

  // Хранит фиксированные решения для данной вершины дерева
  // Пара (i, 0) означает, что мы не берём i-е множество,
  // а пара (i, 1), что берём
  std::vector<std::pair<size_t, bool>> decisions;

  // нижняя оценка на решение в поддереве этой вершины
  double lower_bound;

  // верхняя оценка на решение, получается дополнением решения с помощью
  // жадного алгоритма
  size_t upper_bound;

  // Множество, по которому будет происходить разделение в данной вершине
  size_t branch_set;
};

template <typename Comparator>
class PriorityNodesQueue {
  std::vector<Node> heap_;

  [[no_unique_address]]
  Comparator comparator_;

 public:
  void push(Node node) {
    heap_.push_back(std::move(node));
    std::push_heap(heap_.begin(), heap_.end(), comparator_);
  }

  std::optional<Node> pop() {
    if (heap_.empty()) {
      return std::nullopt;
    }

    std::pop_heap(heap_.begin(), heap_.end(), comparator_);

    Node node = std::move(heap_.back());
    heap_.pop_back();

    return std::move(node);
  }
};

// Первой выбирается вершина с наименьшим lower_bound
struct LowerBoundNodeComparator {
  bool operator()(const Node& x, const Node& y) const {
    return x.lower_bound > y.lower_bound;
  }
};

// Первой выбирается самая неглубокая вершина
struct BreadthNodeComparator {
  bool operator()(const Node& x, const Node& y) const {
    return x.decisions.size() > y.decisions.size();
  }
};

// Первой выбирается вершина с наилучшим жадным решением
struct BestUpperBoundNodeComparator {
  bool operator()(const Node& x, const Node& y) const {
    return x.upper_bound > y.upper_bound;
  }
};

template <typename Comparator>
class PriorityWithDFSNodesQueue {
  std::vector<Node> heap_;

  [[no_unique_address]]
  Comparator comparator_;

  std::optional<Node> lifo_slot_;

  void push_to_heap(Node node) {
    heap_.push_back(std::move(node));
    std::push_heap(heap_.begin(), heap_.end(), comparator_);
  }

 public:
  void push(Node node) {
    if (!lifo_slot_) {
      lifo_slot_ = std::move(node);
    } else if (comparator_(*lifo_slot_, node)) {
      push_to_heap(std::move(*lifo_slot_));
      lifo_slot_ = std::move(node);
    } else {
      push_to_heap(std::move(node));
    }
  }

  std::optional<Node> pop() {
    if (lifo_slot_) {
      Node node = std::move(*lifo_slot_);
      lifo_slot_ = std::nullopt;

      return std::move(node);
    }

    if (heap_.empty()) {
      return std::nullopt;
    }

    std::pop_heap(heap_.begin(), heap_.end(), comparator_);

    Node node = std::move(heap_.back());
    heap_.pop_back();

    return std::move(node);
  }
};

class Constructive {
  const Problem& problem_;
  std::chrono::milliseconds time_limit_;

  // Всякие статистики
  size_t nodes_visited_{0};

  PriorityNodesQueue<BestUpperBoundNodeComparator> queue_;
  std::optional<std::pair<Solution, size_t>> incumbent_;

  CoveringSetsPack pack_;

  // GraphvizDrawer drawer_;

  // Использует жадный алгоритм, чтобы найти какое-то решение, согласованное с
  // decisions. Решение может покрывать не все элементы. Тогда в поддереве нет
  // допустимых решений.
  // Также возвращает индекс множества, которое жадный алгоритм добавил первым,
  // или std::nullopt, если жадный алгоритм ничего не добавил
  std::pair<Solution, std::optional<size_t>> finish_solution(
      std::span<const std::pair<size_t, bool>> decisions) {
    pack_.reset();

    std::vector<size_t> result;
    std::optional<size_t> first_greedy = std::nullopt;

    // Применяем уже сделанные решения
    for (auto [set_index, decision] : decisions) {
      if (decision) {
        result.push_back(set_index);
        pack_.cover_set(set_index);
      } else {
        pack_.remove_set(set_index);
      }
    }

    // Достраиваем жадным способом
    while (true) {
      auto chosen_set = pack_.max_cost_set();

      if (!chosen_set.has_value()) {
        break;
      }

      if (!first_greedy.has_value()) {
        first_greedy = chosen_set->first;
      }

      result.push_back(chosen_set->first);
      pack_.cover_set(chosen_set->first);
    }

    return {Solution{std::move(result)}, first_greedy};
  }

  double get_lower_bound(std::span<const std::pair<size_t, bool>> decisions) {
    return 0;
  }

  void add_drawer_node(size_t id, std::string label, std::string fill,
                       const Node* parent) {
    // drawer_.add_node({
    //     .id = id,
    //     .label = std::move(label),
    //     .fill = std::move(fill),
    // });
    //
    // if (parent != nullptr) {
    //   drawer_.add_edge({
    //       .from = parent->id,
    //       .to = id,
    //   });
    // }
  }

  void push_to_queue(std::vector<std::pair<size_t, bool>> decisions,
                     const Node* parent) {
    ++nodes_visited_;

    auto [solution, first_greedy] = finish_solution(decisions);

    auto evaluation = evaluate(problem_, solution);

    if (!evaluation.is_valid) {
      // Решение нельзя дополнить до корректного, в соответствии с decisions.
      // Обрубаем это поддерево.

      add_drawer_node(nodes_visited_, "infeasible", "red", parent);
      return;
    }

    if (!first_greedy.has_value()) {
      // Дошли до листа дерева
      add_drawer_node(nodes_visited_, "leaf", "green", parent);
      return;
    }

    if (!incumbent_ || evaluation.score < incumbent_->second) {
      incumbent_ = {solution, evaluation.score};
    }

    double lower_bound = get_lower_bound(decisions);

    Node node = {
        .id = nodes_visited_,

        .decisions = std::move(decisions),
        .lower_bound = lower_bound,
        .upper_bound = evaluation.score,

        // разделяемся по множеству, которое было выбрано первым в жадном
        // алгоритме
        .branch_set = *first_greedy,
    };

    if (incumbent_ && incumbent_->second < node.lower_bound) {
      // В данном поддереве нет решений лучше, чем текущий кандидат.
      // Обрубаем это поддерево.
      add_drawer_node(nodes_visited_, "bounds", "orange", parent);

      return;
    }

    add_drawer_node(nodes_visited_,
                    std::format("{}\nlb: {}, ub: {}", node.branch_set,
                                node.lower_bound, node.upper_bound),
                    "grey", parent);

    queue_.push(std::move(node));
  }

 public:
  explicit Constructive(const Problem& problem,
                        std::chrono::milliseconds time_limit)
      : problem_(problem), time_limit_(time_limit), pack_(problem) {}

  std::optional<Solution> solve() {
    auto start_time = std::chrono::high_resolution_clock::now();

    push_to_queue({}, nullptr);

    while (true) {
      auto current_time = std::chrono::high_resolution_clock::now();

      if (duration_cast<std::chrono::milliseconds>(current_time - start_time) >
          time_limit_) {
        break;
      }

      auto node = queue_.pop();

      if (!node) {
        break;
      }

      // left child
      auto left_decisions = node->decisions;
      left_decisions.emplace_back(node->branch_set, false);

      push_to_queue(std::move(left_decisions), &*node);

      // right child
      auto right_decisions = node->decisions;
      right_decisions.emplace_back(node->branch_set, true);

      push_to_queue(std::move(right_decisions), &*node);
    }

    return incumbent_.transform([](const auto& v) { return v.first; });
  }

  size_t get_visited_nodes_count() const { return nodes_visited_; }

  // void draw_tree(std::ostream& os) const { drawer_.draw(os); }
};

}  // namespace setcover
