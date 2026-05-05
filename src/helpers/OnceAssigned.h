#pragma once

#ifdef NDEBUG

template <typename T>
using OnceAssigned = T;

#else

#include <stdexcept>
#include <utility>

template <typename T>
class OnceAssigned {
  T value_;
  bool assigned_{false};

 public:
  OnceAssigned() = default;

  const OnceAssigned& operator=(const T& other) {
    if (assigned_) {
      throw std::runtime_error(
          "Value of type OnceAssigned<T> was assigned for the second time.");
    }

    value_ = other;
    assigned_ = true;

    return *this;
  }

  const OnceAssigned& operator=(T&& other) {
    assert(assigned_ &&
            "Value of type OnceAssigned<T> was assigned for the second time.");

    value_ = std::move(other);
    assigned_ = true;

    return *this;
  }

  operator T() {
    assert(assigned_ &&
           "Value of type OnceAssigned<T> is used before assignment.");

    return value_;
  }

  ~OnceAssigned() {
    assert(assigned_ && "Value of type OnceAssigned<T> was not assigned.");
  }
};

#endif
