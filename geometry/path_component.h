// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <vector>

#include "impeller/geometry/point.h"
#include "impeller/geometry/rect.h"
#include "impeller/geometry/scalar.h"

namespace impeller {

struct SmoothingApproximation {
  Scalar scale;
  Scalar angle_tolerance;
  Scalar cusp_limit;
  Scalar distance_tolerance_square;

  SmoothingApproximation(/* default */)
      : SmoothingApproximation(1.0 /* scale */,
                               0.0 /* angle tolerance */,
                               0.0 /* cusp limit */) {}

  SmoothingApproximation(Scalar p_scale,
                         Scalar p_angle_tolerance,
                         Scalar p_cusp_limit)
      : scale(p_scale),
        angle_tolerance(p_angle_tolerance),
        cusp_limit(p_cusp_limit),
        distance_tolerance_square(0.5 * p_scale * 0.5 * p_scale) {}
};

struct LinearPathComponent {
  Point p1;
  Point p2;

  LinearPathComponent() {}

  LinearPathComponent(Point ap1, Point ap2) : p1(ap1), p2(ap2) {}

  Point Solve(Scalar time) const;

  std::vector<Point> CreatePolyline() const;

  std::vector<Point> Extrema() const;

  bool operator==(const LinearPathComponent& other) const {
    return p1 == other.p1 && p2 == other.p2;
  }
};

struct QuadraticPathComponent {
  Point p1;
  Point cp;
  Point p2;

  QuadraticPathComponent() {}

  QuadraticPathComponent(Point ap1, Point acp, Point ap2)
      : p1(ap1), cp(acp), p2(ap2) {}

  Point Solve(Scalar time) const;

  Point SolveDerivative(Scalar time) const;

  std::vector<Point> CreatePolyline(
      const SmoothingApproximation& approximation) const;

  std::vector<Point> Extrema() const;

  bool operator==(const QuadraticPathComponent& other) const {
    return p1 == other.p1 && cp == other.cp && p2 == other.p2;
  }
};

struct CubicPathComponent {
  Point p1;
  Point cp1;
  Point cp2;
  Point p2;

  CubicPathComponent() {}

  CubicPathComponent(const QuadraticPathComponent& q)
      : p1(q.p1),
        cp1(q.p1 + (q.cp - q.p1) * (2.0 / 3.0)),
        cp2(q.p2 + (q.cp - q.p2) * (2.0 / 3.0)),
        p2(q.p2) {}

  CubicPathComponent(Point ap1, Point acp1, Point acp2, Point ap2)
      : p1(ap1), cp1(acp1), cp2(acp2), p2(ap2) {}

  Point Solve(Scalar time) const;

  Point SolveDerivative(Scalar time) const;

  std::vector<Point> CreatePolyline(
      const SmoothingApproximation& approximation) const;

  std::vector<Point> Extrema() const;

  bool operator==(const CubicPathComponent& other) const {
    return p1 == other.p1 && cp1 == other.cp1 && cp2 == other.cp2 &&
           p2 == other.p2;
  }
};

struct MovePathComponent {
  Point destination;

  MovePathComponent() {}

  MovePathComponent(Point p) : destination(p) {}

  bool operator==(const MovePathComponent& other) const {
    return destination == other.destination;
  }
};

}  // namespace impeller
