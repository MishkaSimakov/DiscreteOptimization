#pragma once

#include <functional>
#include <optional>

#include "linear/FieldTraits.h"

template <typename Field>
class ArithmeticMean {
  Field sum_;
  size_t count_;

 public:
  ArithmeticMean() : sum_(0), count_(0) {}

  Field operator*() const { return sum_ / static_cast<Field>(count_); }

  void record(Field value) {
    sum_ += value;
    ++count_;
  }

  size_t count() const { return count_; }
  Field sum() const { return sum_; }
};

template <typename Field>
class GeometricAverage {
  Field product_;
  size_t count_;

 public:
  GeometricAverage() : product_(1), count_(0) {}

  Field operator*() const {
    if (count_ == 0) {
      return 1;
    }

    return std::pow(product_, 1. / count_);
  }

  void record(Field value) {
    product_ *= value;
    ++count_;
  }

  size_t count() const { return count_; }
  Field product() const { return product_; }
};

template <typename Field, typename Comparator = std::less<Field>>
class Minimum {
  std::optional<Field> minimum_;

  [[no_unique_address]]
  Comparator comparator_;

 public:
  Minimum() : minimum_(std::nullopt) {}

  void record(Field value) {
    if (!minimum_ || comparator_(value, *minimum_)) {
      minimum_ = value;
    }
  }

  void record(std::optional<Field> value) {
    if (value) {
      record(*value);
    }
  }

  bool has_value() const { return minimum_.has_value(); }

  Field operator*() const { return *minimum_; }

  explicit operator std::optional<Field>() const { return minimum_; }

  void reset() { minimum_ = std::nullopt; }
};

template <typename Field, typename Comparator = std::less<Field>>
class Maximum {
  std::optional<Field> maximum_;

  [[no_unique_address]]
  Comparator comparator_;

 public:
  Maximum() : maximum_(std::nullopt) {}

  void record(Field value) {
    if (!maximum_ || comparator_(*maximum_, value)) {
      maximum_ = value;
    }
  }

  void record(std::optional<Field> value) {
    if (value) {
      record(*value);
    }
  }

  bool has_value() const { return maximum_.has_value(); }

  Field operator*() const { return *maximum_; }

  explicit operator std::optional<Field>() const { return maximum_; }

  void reset() { maximum_ = std::nullopt; }
};

// Calculates minimum i, s.t. a_i = min_j a_j
template <typename Field, typename Comparator = std::less<Field>>
class ArgMinimum {
 public:
  struct ArgMinValue {
    size_t index;
    Field value;
  };

 private:
  std::optional<ArgMinValue> minimum_;

  [[no_unique_address]]
  Comparator comparator_;

 public:
  ArgMinimum() : minimum_(std::nullopt) {}

  void record(size_t index, Field value) {
    if (!minimum_) {
      minimum_ = {index, value};
      return;
    }

    if (comparator_(value, minimum_->second)) {
      minimum_ = {index, value};
    } else if (!comparator_(minimum_->second, value) &&
               index < minimum_->first) {
      minimum_ = {index, value};
    }
  }

  void record(size_t index, std::optional<Field> value) {
    if (value) {
      record(index, *value);
    }
  }

  bool has_value() const { return minimum_.has_value(); }

  ArgMinValue operator*() const { return *minimum_; }

  explicit operator std::optional<ArgMinValue>() const { return minimum_; }
};

// Calculates minimum i, s.t. a_i = max_j a_j
template <typename Field, typename Comparator = std::less<Field>>
class ArgMaximum {
 public:
  struct ArgMaxValue {
    size_t index;
    Field value;
  };

 private:
  std::optional<ArgMaxValue> maximum_;

  [[no_unique_address]]
  Comparator comparator_;

 public:
  ArgMaximum() : maximum_(std::nullopt) {}

  void record(size_t index, Field value) {
    if (!maximum_) {
      maximum_ = {index, value};
      return;
    }

    if (comparator_(maximum_->second, value)) {
      maximum_ = {index, value};
    } else if (!comparator_(value, maximum_->second) &&
               index < maximum_->first) {
      maximum_ = {index, value};
    }
  }

  void record(size_t index, std::optional<Field> value) {
    if (value) {
      record(index, *value);
    }
  }

  bool has_value() const { return maximum_.has_value(); }

  ArgMaxValue operator*() const { return *maximum_; }

  explicit operator std::optional<ArgMaxValue>() const { return maximum_; }
};
