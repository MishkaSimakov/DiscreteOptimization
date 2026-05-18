#pragma once

#include <optional>
#include <vector>

#include "TwoOptImprover.h"
#include "facility/Types.h"
#include "facility/solvers/Neighborhood.h"

#include "common/annealing/GeometricCooling.h"
#include "common/annealing/SimulatedAnnealing.h"
#include "facility/solvers/annealing/ChangeCustomerFacility.h"
#include "facility/solvers/annealing/ProblemState.h"
#include "facility/solvers/annealing/SolutionState.h"

namespace facility {

class AnnealingImprover {
  constexpr static annealing::SimulatedAnnealingConfig log_config{
#ifdef NDEBUG
      .verify_gain = false,
#else
      .verify_gain = true,
#endif
  };

  const Problem& problem;
  annealing::SimulatedAnnealing<Problem, ProblemState, Solution, SolutionState,
                                annealing::GeometricCooling>
      annealing_;

  TwoOptImprover two_opt;

 public:
  explicit AnnealingImprover(const Problem& problem)
      : problem(problem), annealing_(problem, log_config), two_opt(problem) {
    annealing_.add<ChangeCustomerFacilityManager>("change_customer_facility",
                                                  1);
  }

  std::vector<size_t> improve(std::vector<size_t> solution,
                              std::vector<double> demands) {
    solution = two_opt.improve(std::move(solution), std::move(demands));

    const double start_temperature = 100;
    const double infeasibility_penalty = 100;

    // auto score_before = get_score(problem, solution);

    const auto improved = annealing_.solve(
        Solution{std::move(solution)},
        annealing::GeometricCooling(start_temperature, start_temperature / 10),
        infeasibility_penalty, std::chrono::milliseconds{5});

    // auto score_after = get_score(problem, improved);
    //
    // if (score_before < score_after) {
    //   std::println("x\t{} ({} -> {})", score_before - score_after,
    //   score_before,
    //                score_after);
    // } else {
    //   std::println("o\t{} ({} -> {})", score_before - score_after,
    //   score_before,
    //                score_after);
    // }

    return improved.facility;
  }
};

}  // namespace facility
