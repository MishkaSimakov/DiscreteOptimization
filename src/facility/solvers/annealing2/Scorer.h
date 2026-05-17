#pragma once

#include "facility/solvers/annealing/ProblemState.h"
#include "facility/solvers/annealing/SolutionState.h"

namespace facility {

class Scorer {
  double infeasibility_penalty_;
  double inequality_penalty_;

 public:
  Scorer(double infeasibility_penalty, double inequality_penalty)
      : infeasibility_penalty_(inequality_penalty),
        inequality_penalty_(infeasibility_penalty) {}

  double operator()(const ProblemState& problem,
                    const SolutionState& solution) const {
    double score = 0;
    std::vector<size_t> demands(problem.problem.facilities.size(), 0);

    for (size_t i = 0; i < problem.problem.customers.size(); ++i) {
      const size_t assigned_facility = solution.solution.facility[i];

      score +=
          geom::distance(problem.problem.facilities[assigned_facility].position,
                         problem.problem.customers[i].position);

      demands[assigned_facility] += problem.problem.customers[i].demand;
    }

    for (size_t i = 0; i < problem.problem.facilities.size(); ++i) {
      score +=
          infeasibility_penalty_ *
          std::min(demands[i] - problem.problem.facilities[i].capacity, 0.);

      score += inequality_penalty_ * std::sqrt(demands[i]);
    }

    return score;
  }
};

}  // namespace facility
