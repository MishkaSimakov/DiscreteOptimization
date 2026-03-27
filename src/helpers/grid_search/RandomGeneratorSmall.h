#pragma once

#include <algorithm>
#include <random>
#include <ranges>
#include <string>
#include <vector>

#include "Generator.h"
#include "SequentialGenerator.h"
#include "Types.h"

namespace gridsearch {

class RandomGeneratorSmall final : public Generator {
  std::default_random_engine random_;
  std::vector<Configuration> unvisited_;

 public:
  explicit RandomGeneratorSmall(const ParametersSpace& parameters)
      : Generator(parameters) {
    auto sequence = SequentialGenerator(parameters);

    while (auto config = sequence.next()) {
      unvisited_.push_back(std::move(*config));
    }

    std::ranges::shuffle(unvisited_, random_);
  }

  std::optional<Configuration> next() override {
    if (unvisited_.empty()) {
      return std::nullopt;
    }

    Configuration result = std::move(unvisited_.back());
    unvisited_.pop_back();

    return result;
  }
};

}  // namespace gridsearch
