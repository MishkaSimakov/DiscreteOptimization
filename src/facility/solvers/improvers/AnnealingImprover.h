#pragma once

#include <optional>
#include <vector>

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
      .log_best = false,
      .log_iteration_end = false,
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

 public:
  explicit AnnealingImprover(const Problem& problem)
      : problem(problem), annealing_(problem, log_config) {
    annealing_.add<ChangeCustomerFacilityManager>("change_customer_facility",
                                                  1);
  }

  std::vector<size_t> improve(std::vector<size_t> solution,
                              std::vector<double> demands) {
    const double start_temperature = 1000;
    const double infeasibility_penalty = 50;

    const auto improved = annealing_.solve(
        Solution{solution},
        annealing::GeometricCooling(start_temperature, 0.9, 10),
        infeasibility_penalty,
        timing::Deadline::after(std::chrono::milliseconds{15}));

    return improved.facility;
  }
};

}  // namespace facility
