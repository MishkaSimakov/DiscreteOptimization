#pragma once

#include <algorithm>
#include <numeric>
#include <random>
#include <unordered_map>

#include "coloring/Types.h"
#include "helpers/Time.h"
#include "utils/Accumulators.h"

namespace coloring {

class TabuCol {
  const Graph& problem;
  timing::Deadline deadline;

  std::default_random_engine random_;

  std::vector<size_t> get_initial_solution(const Graph& problem,
                                           size_t colors_count) {
    const size_t n = problem.adjacent.size();

    std::vector<size_t> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::ranges::shuffle(order, random_);

    std::vector<bool> used(colors_count, false);
    std::vector<size_t> result(n, -1);

    for (const size_t v : order) {
      for (const size_t adj : problem.adjacent[v]) {
        if (result[adj] != -1) {
          used[result[adj]] = true;
        }
      }

      std::optional<size_t> color = std::nullopt;
      for (size_t i = 0; i < colors_count; ++i) {
        if (!used[i]) {
          color = i;
          break;
        }
      }

      if (!color) {
        // assign color randomly
        result[v] = random_() % colors_count;
      } else {
        result[v] = *color;
      }

      for (const size_t adj : problem.adjacent[v]) {
        if (result[adj] != -1) {
          used[result[adj]] = false;
        }
      }
    }

    return result;
  }

  struct SolutionState {
    std::vector<size_t> solution;
    size_t score;

    std::vector<size_t> best_solution;
    size_t best_score;

    std::vector<size_t> tabu;
    std::vector<size_t> clash;

    size_t iteration;
    size_t max_colors;

    //
    size_t& get_clash(size_t node, size_t color) {
      return clash[node * max_colors + color];
    }

    const size_t& get_clash(size_t node, size_t color) const {
      return clash[node * max_colors + color];
    }

    size_t& get_tabu(size_t node, size_t color) {
      return tabu[node * max_colors + color];
    }

    const size_t& get_tabu(size_t node, size_t color) const {
      return tabu[node * max_colors + color];
    }
  };

  std::pair<size_t, size_t> get_move(const SolutionState& state) {
    const size_t n = problem.adjacent.size();

    // second parameter is used to break ties randomly
    ArgMinimum<std::pair<size_t, double>, std::less<>> best_move;
    std::uniform_real_distribution<double> tie_breaker(0, 1);

    size_t clashing_count = 0;

    for (size_t v = 0; v < n; ++v) {
      if (state.get_clash(v, state.solution[v]) == 0) {
        continue;
      }

      ++clashing_count;
      const size_t old_color = state.solution[v];

      for (size_t new_color = 0; new_color < state.max_colors; ++new_color) {
        if (state.solution[v] == new_color) {
          continue;
        }

        const size_t new_score = state.score + state.get_clash(v, new_color) -
                                 state.get_clash(v, old_color);

        if (new_score >= state.best_score &&
            state.get_tabu(v, new_color) >= state.iteration) {
          continue;
        }

        best_move.record(v * state.max_colors + new_color,
                         {new_score, tie_breaker(random_)});
      }
    }

    assert(clashing_count != 0);

    if (!best_move.argmin()) {
      // select random clashing node and move it to random color
      size_t selected = random_() % clashing_count;

      for (size_t v = 0; v < n; ++v) {
        if (state.get_clash(v, state.solution[v]) == 0) {
          continue;
        }

        if (selected == 0) {
          size_t new_color = random_() % (state.max_colors - 1);
          if (new_color >= state.solution[v]) {
            ++new_color;
          }

          return {v, new_color};
        }

        --selected;
      }
    }

    return {*best_move.argmin() / state.max_colors,
            *best_move.argmin() % state.max_colors};
  }

 public:
  TabuCol(const Graph& problem, timing::Deadline deadline)
      : problem(problem), deadline(deadline) {}

  std::optional<Solution> solve(const Solution& initial_solution,
                                size_t colors_count) {
    const size_t n = problem.adjacent.size();

    SolutionState state;

    state.iteration = 1;
    state.max_colors = colors_count;

    state.solution = initial_solution.colors;
    for (size_t i = 0; i < n; ++i) {
      if (state.solution[i] >= colors_count) {
        state.solution[i] = random_() % colors_count;
      }
    }

    state.tabu = std::vector<size_t>(colors_count * n, 0);

    state.clash = std::vector<size_t>(colors_count * n, 0);
    for (size_t i = 0; i < n; ++i) {
      for (const size_t adj : problem.adjacent[i]) {
        ++state.get_clash(i, state.solution[adj]);
      }
    }

    state.score = 0;
    for (size_t i = 0; i < n; ++i) {
      state.score += state.get_clash(i, state.solution[i]);
    }

    assert(state.score % 2 == 0);
    state.score /= 2;

    state.best_solution = state.solution;
    state.best_score = state.score;

    while (!deadline.is_over()) {
      auto [node, new_color] = get_move(state);

      const size_t old_color = state.solution[node];

      assert(new_color != old_color);

      // std::println("    {}: {} -> {} (score: {})", node, old_color, new_color,
                   // state.score);

      state.solution[node] = new_color;
      state.score = state.score + state.get_clash(node, new_color) -
                    state.get_clash(node, old_color);

      state.get_tabu(node, old_color) = state.iteration + 10 * state.score;

      for (const size_t adj : problem.adjacent[node]) {
        --state.get_clash(adj, old_color);
        ++state.get_clash(adj, new_color);
      }

      if (state.score == 0) {
        return Solution{std::move(state.solution)};
      }

      if (state.score < state.best_score) {
        std::println("  [!] new best: {}", state.score);
        state.best_solution = state.solution;
        state.best_score = state.score;
      }

      ++state.iteration;
    }

    return std::nullopt;
  }
};

}  // namespace coloring
