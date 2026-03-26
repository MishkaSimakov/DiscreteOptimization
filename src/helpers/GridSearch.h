#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <random>
#include <vector>

enum class GridSearchStrategy { SEQUENTIAL, RANDOM };

template <typename T>
concept GridSearchConfiguration = requires(T config) {
  // serialize method must return string representation of configuration
  // it must be suitable for a file name
  { config.serialize() } -> std::same_as<std::string>;
};

template <typename T>
concept GridSearchRunResult = requires(T result) {
  // serialize method must return string representation of run result
  { result.serialize() } -> std::same_as<std::string>;
};

template <GridSearchConfiguration Configuration, GridSearchRunResult RunResult>
class GridSearch {
  std::vector<Configuration> configurations_;
  std::function<RunResult(Configuration)> runner_;
  GridSearchStrategy strategy_ = GridSearchStrategy::SEQUENTIAL;

  std::filesystem::path output_directory_;

  std::default_random_engine random_;

  std::optional<Configuration> get_next_configuration() {
    if (configurations_.empty()) {
      return std::nullopt;
    }

    if (strategy_ == GridSearchStrategy::SEQUENTIAL) {
      auto result = configurations_.back();
      configurations_.pop_back();
      return result;
    }

    if (strategy_ == GridSearchStrategy::RANDOM) {
      std::uniform_int_distribution<size_t> distribution(
          0, configurations_.size() - 1);

      size_t index = distribution(random_);
      auto result = configurations_[index];
      configurations_.erase(configurations_.begin() + index);
      return result;
    }

    throw std::runtime_error("Grid search strategy is not implemented");
  }

  void visit_configuration(Configuration config) {
    std::string serialized_config = config.serialize();

    std::println("visiting {}", serialized_config);

    auto result = runner_(config);
    std::string serialized_result = result.serialize();

    std::println("  result: {}", serialized_result);

    std::ofstream output(output_directory_ / serialized_config);
    if (!output) {
      throw std::runtime_error("Failed to open file for storing run result");
    }

    output << serialized_result;
  }

 public:
  explicit GridSearch(std::filesystem::path output_directory)
      : output_directory_(std::move(output_directory)) {}

  void add_configurations(const std::vector<Configuration>& configurations) {
    configurations_.insert(configurations_.end(), configurations.begin(),
                           configurations.end());
  }

  void set_runner(std::function<RunResult(Configuration)> runner) {
    runner_ = std::move(runner);
  }

  void set_strategy(GridSearchStrategy strategy) { strategy_ = strategy; }

  void run() {
    while (auto config = get_next_configuration()) {
      visit_configuration(*config);
    }
  }
};
