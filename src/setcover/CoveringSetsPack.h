#pragma once

#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <vector>

#include "Types.h"
#include "utils/Accumulators.h"

namespace setcover {

// Структура данных для работы жадноподобных алгоритмов
// Для каждого элемента хранит множества, которые его покрывают
class CoveringSetsPack {
  const Problem& problem_;

  std::vector<size_t> sets_;
  std::vector<size_t> begins_;

  std::vector<bool> mask_;
  std::vector<size_t> current_sizes_;

  std::unordered_set<size_t> removed_sets_;

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

  void remove_set(size_t set_index) { removed_sets_.insert(set_index); }

  void cover_set(size_t set_index) {
    for (size_t element : problem_.sets[set_index].elements) {
      if (mask_[element]) {
        mask_[element] = false;

        for (size_t i = begins_[element]; i < begins_[element + 1]; ++i) {
          --current_sizes_[sets_[i]];
        }
      }
    }
  }

  std::optional<double> get_min_covering_cost(size_t element) {
    if (!mask_[element]) {
      // элемент уже покрыт
      return 0;
    }

    Minimum<double> covering_cost;

    for (size_t i = begins_[element]; i < begins_[element + 1]; ++i) {
      if (!removed_sets_.contains(sets_[i])) {
        covering_cost.record(
            static_cast<double>(problem_.sets[sets_[i]].cost) /
            static_cast<double>(problem_.sets[sets_[i]].elements.size()));
      }
    }

    return covering_cost.min();
  }

  std::vector<size_t> get_covering_sets(double threshold) const {
    std::vector<size_t> result;

    for (size_t i = 0; i < problem_.sets.size(); ++i) {
      if (removed_sets_.contains(i)) {
        continue;
      }

      double value = static_cast<double>(current_sizes_[i]) /
                     static_cast<double>(problem_.sets[i].cost);

      if (value >= threshold) {
        result.push_back(i);
      }
    }

    return result;
  }

  std::optional<std::pair<size_t, double>> max_cost_set() const {
    double max_cost_value = 0;
    size_t max_cost_index = 0;

    for (size_t i = 0; i < problem_.sets.size(); ++i) {
      if (removed_sets_.contains(i)) {
        continue;
      }

      double value = static_cast<double>(current_sizes_[i]) /
                     static_cast<double>(problem_.sets[i].cost);

      if (value > max_cost_value) {
        max_cost_value = value;
        max_cost_index = i;
      }
    }

    return max_cost_value > 0
               ? std::optional{std::pair{max_cost_index, max_cost_value}}
               : std::nullopt;
  }

  void reset() {
    std::ranges::fill(mask_, true);
    removed_sets_.clear();

    for (size_t i = 0; i < problem_.sets.size(); ++i) {
      current_sizes_[i] = problem_.sets[i].elements.size();
    }
  }
};

}  // namespace setcover
