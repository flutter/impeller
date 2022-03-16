// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "impeller/geometry/path.h"
#include "impeller/geometry/rect.h"
#include "impeller/geometry/scalar.h"

namespace impeller {

class PathBuilder {
 public:
  /// Used for approximating quarter circle arcs with cubic curves. This is the
  /// control point distance which results in the smallest possible unit circle
  /// integration for a right angle arc. It can be used to approximate arcs less
  /// than 90 degrees to great effect by simply reducing it proportionally to
  /// the angle. However, accuracy rapidly diminishes if magnified for obtuse
  /// angle arcs, and so multiple cubic curves should be used when approximating
  /// arcs greater than 90 degrees.
  constexpr static const Scalar kArcApproximationMagic = 0.551915024494;

  PathBuilder();

  ~PathBuilder();

  Path CopyPath(FillType fill = FillType::kNonZero) const;

  Path TakePath(FillType fill = FillType::kNonZero);

  const Path& GetCurrentPath() const;

  PathBuilder& MoveTo(Point point, bool relative = false);

  PathBuilder& Close();

  PathBuilder& LineTo(Point point, bool relative = false);

  PathBuilder& HorizontalLineTo(Scalar x, bool relative = false);

  PathBuilder& VerticalLineTo(Scalar y, bool relative = false);

  PathBuilder& QuadraticCurveTo(Point controlPoint,
                                Point point,
                                bool relative = false);

  PathBuilder& SmoothQuadraticCurveTo(Point point, bool relative = false);

  PathBuilder& CubicCurveTo(Point controlPoint1,
                            Point controlPoint2,
                            Point point,
                            bool relative = false);

  PathBuilder& SmoothCubicCurveTo(Point controlPoint2,
                                  Point point,
                                  bool relative = false);

  PathBuilder& AddRect(Rect rect);

  PathBuilder& AddCircle(const Point& center, Scalar radius);

  PathBuilder& AddOval(const Rect& rect);

  PathBuilder& AddLine(const Point& p1, const Point& p2);

  PathBuilder& AddQuadraticCurve(Point p1, Point cp, Point p2);

  PathBuilder& AddCubicCurve(Point p1, Point cp1, Point cp2, Point p2);

  struct RoundingRadii {
    Point top_left;
    Point bottom_left;
    Point top_right;
    Point bottom_right;

    RoundingRadii() = default;

    RoundingRadii(Scalar p_top_left,
                  Scalar p_bottom_left,
                  Scalar p_top_right,
                  Scalar p_bottom_right)
        : top_left(p_top_left, p_top_left),
          bottom_left(p_bottom_left, p_bottom_left),
          top_right(p_top_right, p_top_right),
          bottom_right(p_bottom_right, p_bottom_right) {}

    bool AreAllZero() const {
      return top_left.IsZero() &&     //
             bottom_left.IsZero() &&  //
             top_right.IsZero() &&    //
             bottom_right.IsZero();
    }
  };

  PathBuilder& AddRoundedRect(Rect rect, RoundingRadii radii);

  PathBuilder& AddRoundedRect(Rect rect, Scalar radius);

  PathBuilder& AddPath(const Path& path);

 private:
  Point subpath_start_;
  Point current_;
  Path prototype_;
  bool is_moving_ = false;

  Point ReflectedQuadraticControlPoint1() const;

  Point ReflectedCubicControlPoint1() const;

  void MoveToIfNecessary(Point point);

  PathBuilder(const PathBuilder&) = delete;

  PathBuilder& operator=(const PathBuilder&&) = delete;
};

}  // namespace impeller
