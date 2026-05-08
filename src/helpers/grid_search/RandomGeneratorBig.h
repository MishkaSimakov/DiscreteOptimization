#pragma once

#include <algorithm>
#include <random>
#include <ranges>
#include <string>
#include <unordered_set>
#include <vector>

#include "Generator.h"
#include "SequentialGenerator.h"
#include "Types.h"
#include "helpers/Hashers.h"
#include "helpers/Random.h"

namespace gridsearch {

struct vector_hasher {
  template <typename T>
  size_t operator()(const std::vector<T>& vector) const {
    StreamHasher hasher;

    for (const auto& value : vector) {
      hasher << std::hash<T>()(value);
    }

    return hasher.get_hash();
  }
};

class RandomGeneratorBig final : public Generator {
  std::default_random_engine random_;
  std::unordered_set<std::vector<size_t>, vector_hasher> visited_;

  std::vector<std::string> ordered_parameters_;

  std::vector<size_t> random_configuration() {
    std::vector<size_t> result;

    for (const auto& name : ordered_parameters_) {
      size_t domain_size = parameters_.at(name).size();
      size_t index = rnd::index(domain_size, random_);

      result.push_back(index);
    }

    return result;
  }

 public:
  explicit RandomGeneratorBig(const ParametersSpace& parameters)
      : Generator(parameters),
        random_(std::chrono::steady_clock::now().time_since_epoch().count()) {
    for (const auto& name : parameters.domains() | std::views::keys) {
      ordered_parameters_.push_back(name);
    }
  }

  std::optional<Configuration> next() override {
    while (true) {
      auto config_indices = random_configuration();

      auto [itr, inserted] = visited_.emplace(config_indices);

      if (inserted) {
        Configuration result;

        for (size_t i = 0; i < ordered_parameters_.size(); ++i) {
          const auto& name = ordered_parameters_[i];

          result[name] = parameters_.at(name)[config_indices[i]];
        }

        return result;
      }
    }
  }
};

}  // namespace gridsearch
