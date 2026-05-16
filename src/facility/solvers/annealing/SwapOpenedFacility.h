#pragma once

#include <random>
#include <span>

#include "SolutionState.h"
#include "common/annealing/ActionGain.h"
#include "helpers/Random.h"

namespace facility {

struct SwapOpenedFacilityAction {
  // close facility @to_close, open facility @to_open instead
  // move all customers from @to_close to @to_open
  size_t to_close;
  size_t to_open;
};

class SwapOpenedFacilityManager {
 public:
  using Action = SwapOpenedFacilityAction;

 private:
  const ProblemState& problem;

  std::default_random_engine random_;

  size_t choose_opened_facility(const SolutionState& state) {
    return state.opened[rnd::index(state.opened.size(), random_)];
  }

  size_t choose_closed_facility(const SolutionState& state, size_t opened) {
    auto closed = state.closed;

    // take 10 closest closed facilities as candidates
    auto distance_proj = [&](const size_t facility) {
      return geom::distance_sqr(problem.problem.facilities[facility].position,
                                problem.problem.facilities[opened].position);
    };

    const auto nth = closed.size() < 10 ? closed.end() : closed.begin() + 10;
    std::ranges::nth_element(closed, nth, {}, distance_proj);
    std::ranges::sort(closed.begin(), nth, {}, distance_proj);

    for (const size_t facility : std::span{closed.begin(), nth}) {
      if (rnd::bernoulli(0.4, random_)) {
        return facility;
      }
    }

    return closed[0];
  }

 public:
  explicit SwapOpenedFacilityManager(const ProblemState& problem)
      : problem(problem) {}

  std::optional<SwapOpenedFacilityAction> generate(const SolutionState& state,
                                                   annealing::SolverStateDTO) {
    const size_t opened = choose_opened_facility(state);
    const size_t closed = choose_closed_facility(state, opened);

    return SwapOpenedFacilityAction{opened, closed};
  }

  annealing::ActionGain get_gain(const SolutionState& state,
                                 SwapOpenedFacilityAction action) {
    const Facility& to_close = problem.problem.facilities[action.to_close];
    const Facility& to_open = problem.problem.facilities[action.to_open];

    double score_gain = to_close.cost - to_open.cost;

    // distance gain
    for (size_t i = 0; i < problem.problem.customers.size(); ++i) {
      const Customer& customer = problem.problem.customers[i];

      if (state.solution.facility[i] == action.to_close) {
        score_gain += distance(customer.position, to_close.position) -
                      distance(customer.position, to_open.position);
      }
    }

    //
    const double to_close_capacity =
        problem.problem.facilities[action.to_close].capacity;
    const double to_open_capacity =
        problem.problem.facilities[action.to_open].capacity;

    const double demand = to_close_capacity - state.capacity[action.to_close];

    const double infeasibility_gain = std::max(demand - to_close_capacity, 0.) -
                                      std::max(demand - to_open_capacity, 0.);

    return {
        .score = score_gain,
        .infeasibility = infeasibility_gain,
    };
  }

  void apply_action(SolutionState& state, SwapOpenedFacilityAction action,
                    annealing::SolverStateDTO) {
    const double to_close_capacity =
        problem.problem.facilities[action.to_close].capacity;
    const double to_open_capacity =
        problem.problem.facilities[action.to_open].capacity;

    const double demand = to_close_capacity - state.capacity[action.to_close];

    state.capacity[action.to_close] = to_close_capacity;
    state.capacity[action.to_open] = to_open_capacity - demand;

    std::ranges::replace(state.solution.facility, action.to_close,
                         action.to_open);

    for (size_t& v : state.opened) {
      if (v == action.to_close) {
        v = action.to_open;
        break;
      }
    }

    for (size_t& v : state.closed) {
      if (v == action.to_open) {
        v = action.to_close;
        break;
      }
    }

    if (demand <= to_open_capacity != demand <= to_close_capacity) {
      state.update_infeasible_customers();
    }
  }
};

}  // namespace facility
