#pragma once

#include <cassert>
#include <cmath>
#include <vector>

#include "tsp/Types.h"

namespace tsp {

template <bool with_history = false>
class TourStorage {
  struct Node {
    size_t prev;
    size_t next;

    size_t rank;
  };

  std::vector<Node> nodes_;
  std::vector<std::pair<size_t, size_t>> history_;

  // Returns number of nodes on the arc from a to b (a and b included).
  size_t get_arc_length(size_t a, size_t b) const {
    const size_t n = nodes_.size();
    const size_t a_rank = nodes_[a].rank;
    const size_t b_rank = nodes_[b].rank;

    if (b_rank >= a_rank) {
      return b_rank - a_rank + 1;
    }

    return n + b_rank - a_rank + 1;
  }

  // Reverses all edges on arc from a to b, updates ranks.
  void reverse_arc(size_t a, size_t b) {
    const size_t n = size();

    size_t rank = nodes_[a].rank;
    size_t current = b;

    const size_t end = pred(a);

    do {
      const size_t next = nodes_[current].prev;

      nodes_[current].rank = rank;
      std::swap(nodes_[current].next, nodes_[current].prev);

      rank = (rank + 1) % n;
      current = next;
    } while (current != end);
  }

  // (a, succ(a)), (b, succ(b)) are replaced with (a, b), (succ(a), succ(b))
  void apply_2opt_impl(size_t a, size_t b) {
    if (get_arc_length(b, a) < get_arc_length(a, b)) {
      std::swap(a, b);
    }

    if constexpr (with_history) {
      history_.emplace_back(a, b);
    }

    const size_t n = nodes_.size();

    const size_t t1 = a;
    const size_t t2 = succ(a);
    const size_t t3 = succ(b);
    const size_t t4 = b;

    reverse_arc(t2, t4);

    nodes_[t1].next = t4;
    nodes_[t4].prev = t1;
    nodes_[t2].next = t3;
    nodes_[t3].prev = t2;
  }

 public:
  struct Transaction {
   private:
    const size_t size;

    explicit Transaction(size_t size) : size(size) {}

    friend TourStorage;
  };

  explicit TourStorage(const Solution& solution)
      : nodes_(solution.next.size()) {
    size_t current = 0;

    for (size_t i = 0; i < solution.next.size(); ++i) {
      nodes_[current].rank = i;
      nodes_[current].next = solution.next[current];
      nodes_[solution.next[current]].prev = current;

      current = solution.next[current];
    }
  }

  size_t pred(size_t id) const { return nodes_[id].prev; }

  size_t succ(size_t id) const { return nodes_[id].next; }

  // Applies 2-opt to tour.
  // For a given t1, t2, t3 there are two options for t4.
  // One of them would result in 2 cycles, other would yield a new tour.
  // The one that gives new tour is chosen.
  void apply_2opt(size_t t1, size_t t2, size_t t3) {
    assert(pred(t1) == t2 || succ(t1) == t2);

    if (succ(t1) == t2) {
      // t1 -> t2, t4 -> t3
      const size_t t4 = pred(t3);

      apply_2opt_impl(t1, t4);
    } else {
      // t2 -> t1, t3 -> t4
      apply_2opt_impl(t2, t3);
    }
  }

  size_t get_2opt_node(size_t t1, size_t t2, size_t t3) const {
    assert(pred(t1) == t2 || succ(t1) == t2);

    const size_t t4 = succ(t1) == t2 ? pred(t3) : succ(t3);

    return t4;
  }

  bool is_neighbors(size_t a, size_t b) const {
    return succ(a) == b || pred(a) == b;
  }

  void opt(const std::vector<size_t>& path) {
    assert(path.size() % 2 == 0);

    if (path.empty()) {
      return;
    }

    // reverse arcs
    for (size_t i = 0; 2 * i < path.size(); ++i) {
      const size_t a0 = path[2 * i + 0];
      const size_t a1 = path[2 * i + 1];
      const size_t a2 = path[(2 * i + 2) % path.size()];

      if (get_arc_length(a1, a2) > get_arc_length(a1, a0)) {
        reverse_arc(a2, a1);
      }
    }

    // reconnect edges
    for (size_t i = 0; 2 * i < path.size(); ++i) {
      const size_t a0 = path[2 * i + 0];
      const size_t a1 = path[2 * i + 1];

      nodes_[a0].next = a1;
      nodes_[a1].prev = a0;
    }

    size_t current = 0;
    for (size_t i = 0; i < size(); ++i) {
      nodes_[current].rank = i;
      current = succ(current);
    }
  }

  // for debug
  bool is_valid() const {
    const size_t n = nodes_.size();

    size_t current = 0;
    size_t rank = nodes_[current].rank;

    for (size_t i = 0; i < n; ++i) {
      if (nodes_[current].rank != rank ||
          nodes_[nodes_[current].next].prev != current) {
        return false;
      }

      current = nodes_[current].next;
      rank = (rank + 1) % n;
    }

    return true;
  }

  Solution to_solution() const {
    const size_t n = nodes_.size();
    std::vector<size_t> result(n);

    for (size_t i = 0; i < n; ++i) {
      result[i] = nodes_[i].next;
    }

    return Solution{result};
  }

  Transaction start_transaction() const
    requires(with_history)
  {
    return Transaction(history_.size());
  }

  void commit_transaction(Transaction transaction)
    requires(with_history)
  {
    history_.resize(transaction.size);
  }

  void rollback_transaction(Transaction transaction)
    requires(with_history)
  {
    for (size_t i = transaction.size; i < history_.size(); ++i) {
      auto [a, b] = history_[history_.size() - i - 1];

      apply_2opt_impl(a, b);
    }

    history_.resize(transaction.size);
  }

  size_t size() const { return nodes_.size(); }
};

template <bool with_history>
double get_score(const Problem& problem,
                 const TourStorage<with_history>& tour) {
  const size_t n = problem.points.size();

  double score = 0;

  for (size_t i = 0; i < n; ++i) {
    score += distance(problem.points[i], problem.points[tour.succ(i)]);
  }

  return score;
}

template <bool with_history>
size_t distance(const TourStorage<with_history>& left,
                const TourStorage<with_history>& right) {
  assert(left.size() == right.size() &&
         "solutions must belong to the same problem");

  const size_t n = left.size();
  size_t result = 0;

  for (size_t i = 0; i < n; ++i) {
    if (left.succ(i) != right.succ(i) && left.succ(i) != right.pred(i)) {
      ++result;
    }
  }

  return result;
}

}  // namespace tsp
