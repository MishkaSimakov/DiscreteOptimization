#pragma once

#include <cmath>

namespace geom {

template <typename Field>
class Point {
public:
  Field x;
  Field y;
};

template <typename Field>
double norm_sqr(Point<Field> v) {
  return v.x * v.x + v.y * v.y;
}

template <typename Field>
double norm(Point<Field> v) {
  return std::sqrt(v.x * v.x + v.y * v.y);
}

template <typename Field>
double distance_sqr(Point<Field> x, Point<Field> y) {
  const double dx = x.x - y.x;
  const double dy = x.y - y.y;

  return dx * dx + dy * dy;
}

template <typename Field>
double distance(Point<Field> x, Point<Field> y) {
  return std::sqrt(distance_sqr(x, y));
}

}  // namespace geom
