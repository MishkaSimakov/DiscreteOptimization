#pragma once

#include <concepts>

namespace annealing {

template <typename T, typename Problem, typename Solution>
concept Scorer = requires(T scorer, Problem p, Solution s) {
  { scorer(p, s) } -> std::same_as<double>;
};

template <typename T>
concept CoolingProcess = requires(T cooling, double time) {
  { cooling.get_temperature(time) } -> std::same_as<double>;
};

template <typename Problem, typename Solution, typename Scorer,
          typename ActionsPack, typename Cooling>
Solution solve(Problem problem, Solution solution, Scorer scorer,
               ActionsPack actions, Cooling cooling) {}

}  // namespace annealing
