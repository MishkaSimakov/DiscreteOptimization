#pragma once

#include <cassert>
#include <cmath>
#include <vector>

#include "tsp/Types.h"

namespace tsp {

class TourStorage {
  struct Node {
    size_t prev;
    size_t next;

    size_t rank;
  };

  std::vector<Node> nodes_;
  std::vector<std::pair<size_t, size_t>> history_;

  // (a, succ(a)), (b, succ(b)) are replaced with (a, b), (succ(a), succ(b))
  void apply_2opt_impl(size_t a, size_t b) {
    history_.emplace_back(a, b);

    const size_t n = nodes_.size();

    const size_t t1 = a;
    const size_t t2 = succ(a);
    const size_t t3 = succ(b);
    const size_t t4 = b;

    size_t rank = nodes_[t2].rank;
    size_t current = t4;

    do {
      const size_t next = nodes_[current].prev;

      nodes_[current].rank = rank;
      std::swap(nodes_[current].next, nodes_[current].prev);

      rank = (rank + 1) % n;
      current = next;
    } while (current != t1);

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
    size_t rank = 0;

    for (size_t i = 0; i < solution.next.size(); ++i) {
      nodes_[current].rank = rank;
      nodes_[current].next = solution.next[current];
      nodes_[solution.next[current]].prev = current;

      ++rank;
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

  Transaction start_transaction() const { return Transaction(history_.size()); }

  void commit_transaction(Transaction transaction) {
    history_.resize(transaction.size);
  }

  void rollback_transaction(Transaction transaction) {
    for (size_t i = transaction.size; i < history_.size(); ++i) {
      auto [a, b] = history_[history_.size() - i - 1];

      apply_2opt_impl(a, b);
    }

    history_.resize(transaction.size);
  }
};

}  // namespace tsp
