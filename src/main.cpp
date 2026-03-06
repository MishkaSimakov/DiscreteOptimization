#include <iostream>

#include "helpers/Files.h"
#include "helpers/Time.h"
#include "setcover/Evaluator.h"
#include "setcover/Reader.h"
#include "setcover/solvers/Constructive.h"
#include "setcover/solvers/GRASP.h"
#include "setcover/solvers/Greedy.h"
#include "setcover/solvers/MILP.h"

const std::vector<std::string> kGradedProblems = {
    "sc_157_0",  "sc_330_0",   "sc_1000_11",
    "sc_5000_1", "sc_10000_5", "sc_10000_2",
};

void solve(const std::filesystem::path& path) {
  auto problem = setcover::read_problem(path);

  // std::println("solving {}, #elements = {}, #sets = {}",
  //              path.filename().string(), problem.elements_count,
  //              problem.sets.size());
  //
  // auto greedy_solution = setcover::Greedy().solve(problem);
  // auto greedy_evaluation = setcover::evaluate(problem, greedy_solution);
  //
  // assert(greedy_evaluation.is_valid);

  auto grasp_solution =
      setcover::GRASP(0.1, 0.4, std::chrono::seconds{30}).solve(problem);
  auto grasp_evaluation = setcover::evaluate(problem, grasp_solution);

  assert(grasp_evaluation.is_valid);

  // auto constructive_solver =
  //     setcover::Constructive(problem, std::chrono::seconds{30});
  // auto constructive_solution = constructive_solver.solve();
  //
  // // constructive_solver.draw_tree(std::cout);
  //
  // assert(constructive_solution.has_value());
  // auto constructive_evaluation =
  //     setcover::evaluate(problem, *constructive_solution);
  //
  // assert(constructive_evaluation.is_valid);
  //
  // std::println(
  //     "  greedy score: {}, grasp score: {}, constructive score: {} ({} nodes)",
  //     greedy_evaluation.score, grasp_evaluation.score,
  //     constructive_evaluation.score,
  //     constructive_solver.get_visited_nodes_count());
  std::println("  grasp: {}", grasp_evaluation.score);
}

int main() {
  auto duration = timing::timeit([] {
    for (const auto& file : kGradedProblems) {
      solve(files::problem_path(1, file));
    }
  });

  std::println("total duration: {}", duration);

  return 0;
}
