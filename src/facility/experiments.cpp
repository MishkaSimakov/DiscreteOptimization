#include <print>

#include "Evaluator.h"
#include "Output.h"
#include "Reader.h"
#include "helpers/Files.h"
#include "helpers/Time.h"
#include "solvers/AngryCustomers.h"
#include "solvers/Greedy.h"
#include "solvers/GreedyFacilities.h"
#include "solvers/annealing/CloseFacility.h"
#include "solvers/annealing/SimulatedAnnealing.h"

using namespace std::chrono_literals;
using namespace facility;

const std::vector<std::string> graded_problems = {
    // "fl_25_2", "fl_100_1", "fl_200_7",
    "fl_500_7",
    // "fl_1000_2",
    // "fl_2000_2",
};

void solve(const std::string& problem_name) {
  auto path = files::problem_path("facility", problem_name);
  auto problem = read_problem(path);

  std::println("solving {}, #facilities = {}, #customers = {}",
               path.filename().string(), problem.facilities.size(),
               problem.customers.size());

  // auto solution = Greedy(problem).solve();

  GeneticsParameters params{
      .population_size = 100,
      .mutation_rate = 0.5,
      .similarity_replacement_threshold = 2,
  };

  auto solution =
      AngryCustomers(problem, timing::Deadline::after(30s), params).solve();

  std::println("finished genetics, starting SA...");

  auto solver = SimulatedAnnealing(problem, timing::Deadline::after(10min));

  constexpr double open_close_prob = 0.0001;

  solver.add<ChangeCustomerFacilityManager>("change_customer_facility", 0.9);
  solver.add<SwapOpenedFacilityManager>("swap_opened_facility", 0.1);

  // solver.add<OpenFacilityManager>("open_facility", open_close_prob / 2);
  // solver.add<CloseFacilityManager>("close_facility", open_close_prob / 2);

  solution = solver.solve(solution);

  auto evaluation = evaluate(problem, solution);
  if (!evaluation.is_valid) {
    throw std::runtime_error("Invalid solution");
  }

  std::println("  score: {}", evaluation.score);

  Statistics stats(evaluation, 0s);
  Output().store(problem_name, solution, stats);
}

int main() {
  for (const auto& problem_name : graded_problems) {
    solve(problem_name);
  }

  return 0;
}
