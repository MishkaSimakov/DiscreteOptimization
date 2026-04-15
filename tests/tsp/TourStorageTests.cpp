#include <gtest/gtest.h>

#include "tsp/Types.h"
#include "tsp/solvers/TourStorage.h"

using namespace tsp;

Solution simple_loop(size_t size) {
  std::vector<size_t> result(size);

  for (size_t i = 0; i < size; ++i) {
    result[i] = (i + 1) % size;
  }

  return Solution{result};
}

bool check_tour(const TourStorage<>& tour, const std::vector<size_t>& order) {
  if (tour.size() != order.size()) {
    return false;
  }

  bool reversed = false;
  if (tour.succ(order[0]) != order[1]) {
    reversed = true;
  }

  for (size_t i = 0; i < tour.size(); ++i) {
    const size_t curr = order[i];
    const size_t next = order[(i + 1) % tour.size()];

    if ((!reversed && tour.succ(curr) != next) ||
        (reversed && tour.pred(curr) != next)) {
      return false;
    }
  }

  return true;
}

TEST(TourStorageTests, constructor) {
  auto loop = simple_loop(4);

  const TourStorage tour(loop);
  ASSERT_TRUE(tour.is_valid());

  ASSERT_EQ(tour.succ(0), 1);
  ASSERT_EQ(tour.succ(1), 2);
  ASSERT_EQ(tour.succ(2), 3);
  ASSERT_EQ(tour.succ(3), 0);
}

TEST(TourStorageTests, apply_2opt_1) {
  auto loop = simple_loop(4);

  TourStorage tour(loop);
  tour.apply_2opt(0, 1, 3);

  ASSERT_TRUE(tour.is_valid());

  ASSERT_EQ(tour.succ(0), 2);
  ASSERT_EQ(tour.succ(2), 1);
  ASSERT_EQ(tour.succ(1), 3);
  ASSERT_EQ(tour.succ(3), 0);
}

TEST(TourStorageTests, apply_2opt_2) {
  auto loop = simple_loop(6);

  TourStorage tour(loop);
  tour.apply_2opt(1, 2, 5);

  ASSERT_TRUE(tour.is_valid());

  ASSERT_EQ(tour.succ(0), 1);
  ASSERT_EQ(tour.succ(1), 4);
  ASSERT_EQ(tour.succ(4), 3);
  ASSERT_EQ(tour.succ(3), 2);
  ASSERT_EQ(tour.succ(2), 5);
  ASSERT_EQ(tour.succ(5), 0);
}

TEST(TourStorageTests, apply_2opt_3) {
  auto loop = simple_loop(7);

  TourStorage tour(loop);
  tour.apply_2opt(5, 6, 2);

  ASSERT_TRUE(tour.is_valid());

  ASSERT_EQ(tour.succ(5), 2);
  ASSERT_EQ(tour.succ(2), 1);
  ASSERT_EQ(tour.succ(1), 0);
  ASSERT_EQ(tour.succ(0), 6);
  ASSERT_EQ(tour.succ(6), 3);
  ASSERT_EQ(tour.succ(3), 4);
  ASSERT_EQ(tour.succ(4), 5);
}

TEST(TourStorageTests, apply_opt_1) {
  auto loop = simple_loop(7);

  TourStorage tour(loop);
  tour.opt({0, 4, 1, 5});

  ASSERT_TRUE(tour.is_valid());
  ASSERT_TRUE(check_tour(tour, {0, 4, 3, 2, 1, 5, 6}));
}

TEST(TourStorageTests, apply_opt_2) {
  auto loop = simple_loop(17);

  TourStorage tour(loop);
  tour.opt({0, 11, 13, 10, 10, 3, 9, 16, 14, 2});

  ASSERT_TRUE(tour.is_valid());
  ASSERT_TRUE(check_tour(
      tour, {0, 11, 12, 13, 10, 3, 4, 5, 6, 7, 8, 9, 16, 15, 14, 2, 1}));
}
