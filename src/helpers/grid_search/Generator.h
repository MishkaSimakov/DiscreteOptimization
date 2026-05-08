#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Types.h"

namespace gridsearch {

class Generator {
 protected:
  const ParametersSpace& parameters_;

 public:
  explicit Generator(const ParametersSpace& parameters)
      : parameters_(parameters) {}

  virtual std::optional<Configuration> next() = 0;

  virtual ~Generator() = default;
};

}  // namespace gridsearch
