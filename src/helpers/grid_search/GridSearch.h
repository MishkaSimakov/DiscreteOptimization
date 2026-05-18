#pragma once

#include <expected>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>
#include <print>
#include <random>
#include <thread>
#include <vector>

#include "Generator.h"
#include "RandomGeneratorBig.h"
#include "RandomGeneratorSmall.h"
#include "SequentialGenerator.h"
#include "Types.h"

namespace gridsearch {

template <typename RunResult>
class GridSearch {
  ParametersSpace parameters_;
  std::function<RunResult(const Configuration&)> runner_;

  GridSearchStrategy strategy_ = GridSearchStrategy::SEQUENTIAL;

  std::mutex mutex_;
  size_t index_{0};
  std::filesystem::path output_directory_;

  std::optional<size_t> max_threads_;

  static std::string stringify_config(const Configuration& config) {
    nlohmann::json json = config;
    return json.dump();
  }

  void visit_configuration(const Configuration& config) {
    std::println("visiting {}", stringify_config(config));

    try {
      auto result = runner_(config);

      store_result(config, std::move(result));
    } catch (const std::exception& exception) {
      store_result(config, std::unexpected{exception.what()});
    }
  }

  static std::string get_filename(size_t index) {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();

    return std::format("{}_{}.json", now.count(), index);
  }

  void store_result(const Configuration& config,
                    std::expected<RunResult, std::string> result) {
    nlohmann::json json = config;

    if (result.has_value()) {
      json["status"] = "success";
      json["result"] = result.value();
    } else {
      json["status"] = "error";
      json["error"] = result.error();
    }

    std::lock_guard guard(mutex_);

    std::ofstream output(output_directory_ / get_filename(index_));
    if (!output) {
      throw std::runtime_error("Failed to open file for storing run result");
    }

    output << json.dump(2);

    ++index_;
  }

  std::optional<Configuration> get_next_configuration(Generator& generator) {
    std::lock_guard guard(mutex_);

    return generator.next();
  }

  void worker(Generator& generator) {
    while (auto config = get_next_configuration(generator)) {
      visit_configuration(*config);
    }
  }

  std::unique_ptr<Generator> get_generator() const {
    if (strategy_ == GridSearchStrategy::SEQUENTIAL) {
      return std::make_unique<SequentialGenerator>(parameters_);
    }
    if (strategy_ == GridSearchStrategy::RANDOM) {
      if (parameters_.configurations_count() > 40'000) {
        return std::make_unique<RandomGeneratorBig>(parameters_);
      }

      return std::make_unique<RandomGeneratorSmall>(parameters_);
    }

    throw std::runtime_error("Unknown grid search strategy");
  }

 public:
  explicit GridSearch(std::filesystem::path output_directory)
      : output_directory_(std::move(output_directory)) {}

  void add_parameter(std::string name, std::vector<double> values) {
    parameters_.add_parameter(std::move(name), std::move(values));
  }

  void set_runner(std::function<RunResult(const Configuration&)> runner) {
    runner_ = std::move(runner);
  }

  void set_strategy(GridSearchStrategy strategy) { strategy_ = strategy; }

  // std::nullopt means no limit on threads count
  void set_max_threads(std::optional<size_t> max_threads) {
    max_threads_ = max_threads;
  }

  void start() {
    size_t threads_count = std::thread::hardware_concurrency();

    if (max_threads_) {
      threads_count = std::min(threads_count, *max_threads_);
    }

    std::println("Starting grid search.");
    std::println("configurations = {}", parameters_.configurations_count());
    std::println("threads = {}", threads_count);

    auto generator = get_generator();

    std::vector<std::thread> threads;
    for (size_t i = 0; i < threads_count; ++i) {
      threads.emplace_back([this, &generator] { worker(*generator); });
    }

    for (size_t i = 0; i < threads_count; ++i) {
      threads[i].join();
    }
  }
};

}  // namespace gridsearch
