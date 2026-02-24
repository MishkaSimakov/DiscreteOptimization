#include <iostream>

#include "helpers/Files.h"
#include "helpers/Time.h"
#include "setcover/Evaluator.h"
#include "setcover/Reader.h"
#include "setcover/solvers/Greedy.h"
#include "setcover/solvers/MILP.h"

void solve(const std::filesystem::path& path) {
  auto problem = setcover::read_problem(path);

  std::println("solving {}, #elements = {}, #sets = {}",
               path.filename().string(), problem.elements_count,
               problem.sets.size());

  auto solution = setcover::Greedy().solve(problem);
  auto result = setcover::evaluate(problem, solution);

  std::println("  is valid: {}, score: {}", result.is_valid, result.score);
}

int main() {
  auto duration = timing::timeit([] {
    for (const auto& file :
         files::problems_iterator(1) | std::views::take(10)) {
      if (file.path().filename() == "sc_8661_1") {
        continue;
      }

      solve(file.path());
    }
  });

  std::println("total duration: {}", duration);

  return 0;
}

// 8107