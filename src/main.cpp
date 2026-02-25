#include <iostream>

#include "helpers/Files.h"
#include "helpers/Time.h"
#include "setcover/Evaluator.h"
#include "setcover/Reader.h"
#include "setcover/solvers/Constructive.h"
#include "setcover/solvers/Greedy.h"
#include "setcover/solvers/MILP.h"

const std::vector<std::string> kGradedProblems = {
    "sc_157_0",  "sc_330_0",   "sc_1000_11",
    "sc_5000_1", "sc_10000_5", "sc_10000_2",
};

void solve(const std::filesystem::path& path) {
  auto problem = setcover::read_problem(path);

  std::println("solving {}, #elements = {}, #sets = {}",
               path.filename().string(), problem.elements_count,
               problem.sets.size());

  auto greedy_solution = setcover::Greedy().solve(problem);
  auto greedy_evaluation = setcover::evaluate(problem, greedy_solution);

  assert(greedy_evaluation.is_valid);

  auto constructive_solver =
      setcover::Constructive(problem, std::chrono::seconds{30});
  auto constructive_solution = constructive_solver.solve();
  assert(constructive_solution.has_value());
  auto constructive_evaluation =
      setcover::evaluate(problem, *constructive_solution);

  assert(constructive_evaluation.is_valid);

  std::println("  greedy score: {}, constructive score: {} ({} nodes)",
               greedy_evaluation.score, constructive_evaluation.score,
               constructive_solver.get_visited_nodes_count());
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

// sets are sorted by size in descending order
// BestGreedyNode
// full lower bound calculation
// Отрезание глубоких ветвей сильно ухудшает результат
// solving sc_157_0, #elements = 29, #sets = 157
//   greedy score: 102100, constructive score: 95200 (7093805 nodes)
// solving sc_330_0, #elements = 1023, #sets = 330
//   greedy score: 30, constructive score: 25 (165223 nodes)
// solving sc_1000_11, #elements = 200, #sets = 1000
//   greedy score: 170, constructive score: 149 (491971 nodes)
// solving sc_5000_1, #elements = 500, #sets = 5000
//   greedy score: 36, constructive score: 32 (49207 nodes)
// solving sc_10000_5, #elements = 1000, #sets = 10000
//   greedy score: 76, constructive score: 71 (19925 nodes)
// solving sc_10000_2, #elements = 1000, #sets = 10000
//   greedy score: 192, constructive score: 186 (15175 nodes)
// total duration: 180908ms