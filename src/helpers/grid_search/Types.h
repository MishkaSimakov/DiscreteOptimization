#pragma once

#include <map>
#include <string>

namespace gridsearch {

enum class GridSearchStrategy { SEQUENTIAL, RANDOM };

using Configuration = std::map<std::string, double>;

class ParametersSpace {
  std::map<std::string, std::vector<double>> domains_;

 public:
  size_t configurations_count() const {
    if (domains_.empty()) {
      return 0;
    }

    size_t count = 1;

    for (const auto& values : domains_ | std::views::values) {
      count *= values.size();
    }

    return count;
  }

  void add_parameter(std::string name, std::vector<double> domain) {
    if (domain.empty()) {
      throw std::runtime_error("Domain must be non-empty");
    }

    auto [_, inserted] = domains_.emplace(std::move(name), std::move(domain));

    if (!inserted) {
      throw std::runtime_error("Parameter with this name already exists");
    }
  }

  const std::vector<double>& at(const std::string& name) const {
    return domains_.at(name);
  }

  const auto& domains() const { return domains_; }
};

}  // namespace gridsearch
