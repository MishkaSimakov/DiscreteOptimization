#pragma once

#include "../Types.h"
#include "linear/bb/FullStrongBranching.h"
#include "linear/problem/MILPProblem.h"
#include "linear/problem/ToMatrices.h"
#include "linear/problem/optimization/FullOptimizer.h"

namespace setcover {

class MILP {
  static MILPProblem<double> to_milp(const Problem& problem) {
    MILPProblem<double> result;

    std::vector<Variable<double>> variables;
    for (size_t i = 0; i < problem.sets.size(); ++i) {
      variables.push_back(result.new_variable(std::format("x_{}", i),
                                              VariableType::INTEGER, 0, 1));
    }

    for (size_t j = 0; j < problem.elements_count; ++j) {
      Expression<double> constraint;

      for (size_t i = 0; i < problem.sets.size(); ++i) {
        if (problem.sets[i].elements.contains(j)) {
          constraint += variables[i];
        }
      }

      result.add_constraint(constraint >= Expression<double>{1});
    }

    Expression<double> objective;
    for (size_t i = 0; i < problem.sets.size(); ++i) {
      objective += static_cast<double>(problem.sets[i].cost) * variables[i];
    }
    result.set_objective(-objective);

    return result;
  }

 public:
  MILP() = default;

  Solution solve(const Problem& problem) {
    auto milp_problem = to_milp(problem);

    auto optimizer = TransformToEqualities<double>();
    auto optimized = optimizer.apply(milp_problem);
    auto matrices = to_matrices(optimized);

    auto solver = FullStrongBranchingBranchAndBound(
        matrices.A, matrices.b, matrices.c, matrices.lower, matrices.upper,
        matrices.variables);
    auto result = solver.solve();

    auto* solution = std::get_if<FiniteMILPSolution<double>>(&result.solution);

    if (solution == nullptr) {
      throw std::runtime_error("Failed to find solution via BB");
    }

    auto chosen_mask = optimizer.inverse(solution->point);

    assert((chosen_mask.shape() == std::pair{problem.sets.size(), 1}));

    std::vector<size_t> chosen_sets;
    for (size_t i = 0; i < problem.sets.size(); ++i) {
      if (std::abs(chosen_mask[i, 0] - 1) < 1e-10) {
        chosen_sets.push_back(i);
      }
    }

    return Solution{std::move(chosen_sets)};
  }
};

}  // namespace setcover
