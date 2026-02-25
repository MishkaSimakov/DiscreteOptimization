#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <numeric>
#include <queue>
#include <span>
#include <vector>

#include "setcover/Evaluator.h"
#include "setcover/Types.h"

namespace setcover {

class CoveringSetsPack {
  const Problem& problem_;

  std::vector<size_t> sets_;
  std::vector<size_t> begins_;

  std::vector<bool> mask_;
  std::vector<size_t> current_sizes_;

 public:
  explicit CoveringSetsPack(const Problem& problem)
      : problem_(problem),
        begins_(problem.elements_count + 1),
        mask_(problem.elements_count, true),
        current_sizes_(problem.sets.size()) {
    for (size_t i = 0; i < problem.elements_count; ++i) {
      begins_[i] = sets_.size();

      for (size_t j = 0; j < problem.sets.size(); ++j) {
        if (problem.sets[j].elements.contains(i)) {
          sets_.push_back(j);
        }
      }
    }

    begins_[problem.elements_count] = sets_.size();

    for (size_t i = 0; i < problem.sets.size(); ++i) {
      current_sizes_[i] = problem.sets[i].elements.size();
    }
  }

  void cover_element(size_t element) {
    if (mask_[element]) {
      mask_[element] = false;

      for (size_t i = begins_[element]; i < begins_[element + 1]; ++i) {
        --current_sizes_[sets_[i]];
      }
    }
  }

  bool is_covered(size_t element) const { return mask_[element]; }

  std::span<const size_t> get_covering_sets(size_t element) const {
    return std::span{sets_.begin() + begins_[element],
                     sets_.begin() + begins_[element + 1]};
  }

  std::optional<size_t> max_cost_set() const {
    double max_cost_value = 0;
    size_t max_cost_index = 0;

    for (size_t i = 0; i < problem_.sets.size(); ++i) {
      double value = static_cast<double>(current_sizes_[i]) /
                     static_cast<double>(problem_.sets[i].cost);

      if (value > max_cost_value) {
        max_cost_value = value;
        max_cost_index = i;
      }
    }

    return max_cost_value > 0 ? std::optional{max_cost_index} : std::nullopt;
  }

  void reset() {
    std::ranges::fill(mask_, true);

    for (size_t i = 0; i < problem_.sets.size(); ++i) {
      current_sizes_[i] = problem_.sets[i].elements.size();
    }
  }
};

class Constructive {
  struct Node {
    // уже принятые решения
    // 0 - не взяли множество, 1 - взяли
    std::vector<bool> decisions;

    // нижняя оценка на решение в поддереве этой вершины
    double lower_bound;

    // верхняя оценка на решение, получается дополнением решения с помощью
    // жадного алгоритма
    size_t upper_bound;
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
  struct BestGreedyNode {
    bool operator()(const Node& x, const Node& y) const {
      return x.upper_bound > y.upper_bound;
    }
  };

  const Problem& problem_;
  std::chrono::milliseconds time_limit_;

  // Всякие статистики
  size_t nodes_visited_{0};

  // Порядок, в котором перебираются множества.
  // sets_order_[0] - индекс множества, которое будет рассмотрено первым
  std::vector<size_t> sets_order_;

  // Наверху кучи лежит вершина с наименьшим lower_bound
  std::priority_queue<Node, std::vector<Node>, BestGreedyNode> queue_;
  std::optional<std::pair<Solution, size_t>> incumbent_;

  CoveringSetsPack pack_;

  static std::vector<size_t> get_sets_order(const Problem& problem) {
    std::vector<size_t> result(problem.sets.size());
    std::iota(result.begin(), result.end(), 0);

    std::ranges::sort(result, {}, [&problem](size_t index) {
      return -static_cast<int>(problem.sets[index].elements.size());
    });

    return std::move(result);
  }

  // Использует жадный алгоритм, чтобы найти какое-то решение, согласованное с
  // decisions. Решение может покрывать не все элементы. Тогда в поддереве нет
  // допустимых решений.
  Solution finish_solution(const std::vector<bool>& decisions) {
    pack_.reset();

    std::vector<size_t> result;

    // Применяем уже сделанные решения
    for (size_t i = 0; i < decisions.size(); ++i) {
      size_t set_index = sets_order_[i];

      if (decisions[i]) {
        // взяли множество sets_order_[i], надо убрать из оставшихся его
        // элементы
        result.push_back(set_index);

        for (size_t element : problem_.sets[set_index].elements) {
          pack_.cover_element(element);
        }
      }
    }

    // Достраиваем жадным способом
    while (true) {
      auto chosen_set = pack_.max_cost_set();

      if (!chosen_set.has_value()) {
        break;
      }

      result.push_back(*chosen_set);

      for (size_t element : problem_.sets[*chosen_set].elements) {
        pack_.cover_element(element);
      }
    }

    return Solution{std::move(result)};
  }

  std::optional<Node> pop_node() {
    if (queue_.empty()) {
      return std::nullopt;
    }

    auto node = queue_.top();
    queue_.pop();

    return std::move(node);
  }

  double get_lower_bound(const std::vector<bool>& decisions) {
    pack_.reset();

    double result = 0;

    for (size_t i = 0; i < decisions.size(); ++i) {
      if (decisions[i]) {
        for (size_t element : problem_.sets[sets_order_[i]].elements) {
          pack_.cover_element(element);
        }

        result += static_cast<double>(problem_.sets[sets_order_[i]].cost);
      }
    }

    // Проходим по непокрытым элементам. Для каждого считаем:
    // min c_j / |A_j|

    for (size_t i = 0; i < problem_.elements_count; ++i) {
      if (pack_.is_covered(i)) {
        continue;
      }

      double min = 1e10;  // +infinity

      for (size_t set_index : pack_.get_covering_sets(i)) {
        const auto& set = problem_.sets[set_index];
        min = std::min(min, static_cast<double>(set.cost) /
                                static_cast<double>(set.elements.size()));
      }

      result += min;
    }

    return result;
  }

  void push_to_queue(std::vector<bool> decisions) {
    ++nodes_visited_;

    Solution solution = finish_solution(decisions);

    auto evaluation = evaluate(problem_, solution);

    if (!evaluation.is_valid) {
      // Решение нельзя дополнить до корректного, в соответствии с decisions.
      // Обрубаем это поддерево.
      return;
    }

    if (!incumbent_ || evaluation.score < incumbent_->second) {
      incumbent_ = {solution, evaluation.score};
    }

    double lower_bound = get_lower_bound(decisions);

    Node node = {
        .decisions = std::move(decisions),
        .lower_bound = lower_bound,
        .upper_bound = evaluation.score,
    };

    if (incumbent_ && incumbent_->second < node.lower_bound) {
      // В данном поддереве нет решений лучше, чем текущий кандидат.
      // Обрубаем это поддерево.
      return;
    }

    queue_.push(std::move(node));
  }

 public:
  explicit Constructive(const Problem& problem,
                        std::chrono::milliseconds time_limit)
      : problem_(problem),
        time_limit_(time_limit),
        sets_order_(get_sets_order(problem)),
        pack_(problem) {}

  std::optional<Solution> solve() {
    auto start_time = std::chrono::high_resolution_clock::now();

    push_to_queue({});

    while (true) {
      auto current_time = std::chrono::high_resolution_clock::now();

      if (duration_cast<std::chrono::milliseconds>(current_time - start_time) >
          time_limit_) {
        break;
      }

      auto node = pop_node();

      if (!node) {
        break;
      }

      // Дошли до листа дерева, переходим к следующей вершине
      if (node->decisions.size() == problem_.sets.size()) {
        continue;
      }

      // left child
      auto left_decisions = node->decisions;
      left_decisions.push_back(false);

      push_to_queue(std::move(left_decisions));

      // right child
      auto right_decisions = node->decisions;
      right_decisions.push_back(true);

      push_to_queue(std::move(right_decisions));
    }

    return incumbent_.transform([](const auto& v) { return v.first; });
  }

  size_t get_visited_nodes_count() const { return nodes_visited_; }
};

}  // namespace setcover
