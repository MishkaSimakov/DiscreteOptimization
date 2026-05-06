#pragma once

#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#include "Generator.h"
#include "Types.h"

namespace gridsearch {

class SequentialGenerator final : public Generator {
  std::vector<std::pair<std::string, size_t>> current_;

 public:
  explicit SequentialGenerator(const ParametersSpace& parameters)
      : Generator(parameters) {
    for (const auto& name : parameters.domains() | std::views::keys) {
      current_.emplace_back(name, 0);
    }
  }

  std::optional<Configuration> next() override {
    Configuration result;
    bool incremented = false;

    for (auto& [name, index] : current_) {
      result[name] = parameters_.at(name)[index];

      if (!incremented) {
        if (index + 1 == parameters_.at(name).size()) {
          index = 0;
        } else {
          ++index;
          incremented = true;
        }
      }
    }

    if (!incremented) {
      return std::nullopt;
    }

    return result;
  }
};

}  // namespace gridsearch
