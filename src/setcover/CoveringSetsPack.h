#pragma once

#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <vector>

#include "Types.h"
#include "utils/Accumulators.h"

namespace setcover {

enum class SetState { DEFAULT, INCLUDED, EXCLUDED };

// Структура данных для работы жадноподобных алгоритмов
// Для каждого элемента хранит множества, которые его покрывают
class CoveringSetsPack {
  const Problem& problem_;

  // for each element stores sets covering it
  std::vector<size_t> sets_;
  std::vector<size_t> begins_;

  // count of sets covering given element
  std::vector<size_t> covering_sets_count_;

  // current size of each set (excluding already covered elements)
  std::vector<size_t> current_sizes_;

  // for each set stores its state
  std::vector<SetState> states_;

 public:
  explicit CoveringSetsPack(const Problem& problem)
      : problem_(problem),
        begins_(problem.elements_count + 1),
        covering_sets_count_(problem.elements_count, 0),
        current_sizes_(problem.sets.size()),
        states_(problem.sets.size(), SetState::DEFAULT) {
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

  void default_set(size_t set_index) {
    assert(states_[set_index] == SetState::INCLUDED);

    states_[set_index] = SetState::DEFAULT;

    for (const size_t element : problem_.sets[set_index].elements) {
      --covering_sets_count_[element];

      if (covering_sets_count_[element] == 0) {
        for (const size_t set : get_sets_covering(element)) {
          ++current_sizes_[set];
        }
      }
    }
  }

  void include_set(size_t set_index) {
    assert(states_[set_index] == SetState::DEFAULT);

    states_[set_index] = SetState::INCLUDED;

    for (const size_t element : problem_.sets[set_index].elements) {
      if (covering_sets_count_[element] == 0) {
        for (const size_t set : get_sets_covering(element)) {
          --current_sizes_[set];
        }
      }

      ++covering_sets_count_[element];
    }
  }

  bool is_covered(size_t element) const {
    return covering_sets_count_[element] != 0;
  }

  SetState get_set_state(size_t set) const { return states_[set]; }

  double get_relative_cost(size_t set) const {
    return static_cast<double>(problem_.sets[set].cost) /
           static_cast<double>(problem_.sets[set].elements.size());
  }

  std::span<const size_t> get_sets_covering(size_t element) const {
    return std::span{sets_.begin() + begins_[element],
                     sets_.begin() + begins_[element + 1]};
  }

  std::optional<double> get_min_covering_cost(size_t element) {
    if (covering_sets_count_[element] != 0) {
      // элемент уже покрыт
      return 0;
    }

    Minimum<double> covering_cost;

    for (const size_t set : get_sets_covering(element)) {
      if (states_[set] == SetState::DEFAULT) {
        covering_cost.record(
            static_cast<double>(problem_.sets[set].cost) /
            static_cast<double>(problem_.sets[set].elements.size()));
      }
    }

    return covering_cost.min();
  }

  // Returns all sets that are not included or excluded.
  // Only sets with relative cost >= threshold are returned.
  std::vector<size_t> get_default_sets(double threshold) const {
    std::vector<size_t> result;

    for (size_t i = 0; i < problem_.sets.size(); ++i) {
      if (states_[i] != SetState::DEFAULT || current_sizes_[i] == 0) {
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
      if (states_[i] != SetState::DEFAULT) {
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
    std::ranges::fill(covering_sets_count_, 0);
    std::ranges::fill(states_, SetState::DEFAULT);

    for (size_t i = 0; i < problem_.sets.size(); ++i) {
      current_sizes_[i] = problem_.sets[i].elements.size();
    }
  }
};

}  // namespace setcover
