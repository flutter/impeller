// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>
#include "impeller/geometry/geometry_unittests.h"
#include <limits>
#include "flutter/testing/testing.h"
#include "impeller/geometry/path.h"
#include "impeller/geometry/path_builder.h"
#include "impeller/geometry/path_component.h"
#include "impeller/geometry/point.h"
#include "impeller/geometry/rect.h"
#include "impeller/geometry/scalar.h"
#include "impeller/geometry/size.h"

namespace impeller {
namespace testing {

TEST(GeometryTest, ScalarNearlyEqual) {
  ASSERT_FALSE(ScalarNearlyEqual(0.002f, 0.001f));
  ASSERT_TRUE(ScalarNearlyEqual(0.002f, 0.001f, 0.0011f));
  ASSERT_FALSE(ScalarNearlyEqual(0.002f, 0.001f, 0.0009f));
  ASSERT_TRUE(
      ScalarNearlyEqual(1.0f, 1.0f + std::numeric_limits<float>::epsilon()*4));
}

TEST(GeometryTest, RotationMatrix) {
  auto rotation = Matrix::MakeRotationZ(Radians{M_PI_4});
  auto expect = Matrix{0.707,  0.707, 0, 0,  //
                       -0.707, 0.707, 0, 0,  //
                       0,      0,     1, 0,  //
                       0,      0,     0, 1};
  ASSERT_MATRIX_NEAR(rotation, expect);
}

TEST(GeometryTest, InvertMultMatrix) {
  auto rotation = Matrix::MakeRotationZ(Radians{M_PI_4});
  auto invert = rotation.Invert();
  auto expect = Matrix{0.707, -0.707, 0, 0,  //
                       0.707, 0.707,  0, 0,  //
                       0,     0,      1, 0,  //
                       0,     0,      0, 1};
  ASSERT_MATRIX_NEAR(invert, expect);
}

TEST(GeometryTest, MutliplicationMatrix) {
  auto rotation = Matrix::MakeRotationZ(Radians{M_PI_4});
  auto invert = rotation.Invert();
  ASSERT_MATRIX_NEAR(rotation * invert, Matrix{});
}

TEST(GeometryTest, DeterminantTest) {
  auto matrix = Matrix{3, 4, 14, 155, 2, 1, 3, 4, 2, 3, 2, 1, 1, 2, 4, 2};
  ASSERT_EQ(matrix.GetDeterminant(), -1889);
}

TEST(GeometryTest, InvertMatrix) {
  auto inverted = Matrix{10,  -9,  -12, 8,   //
                         7,   -12, 11,  22,  //
                         -10, 10,  3,   6,   //
                         -2,  22,  2,   1}
                      .Invert();

  auto result = Matrix{
      438.0 / 85123.0,   1751.0 / 85123.0, -7783.0 / 85123.0, 4672.0 / 85123.0,
      393.0 / 85123.0,   -178.0 / 85123.0, -570.0 / 85123.0,  4192 / 85123.0,
      -5230.0 / 85123.0, 2802.0 / 85123.0, -3461.0 / 85123.0, 962.0 / 85123.0,
      2690.0 / 85123.0,  1814.0 / 85123.0, 3896.0 / 85123.0,  319.0 / 85123.0};

  ASSERT_MATRIX_NEAR(inverted, result);
}

TEST(GeometryTest, TestDecomposition) {
  auto rotated = Matrix::MakeRotationZ(Radians{M_PI_4});

  auto result = rotated.Decompose();

  ASSERT_TRUE(result.has_value());

  MatrixDecomposition res = result.value();

  auto quaternion = Quaternion{{0.0, 0.0, 1.0}, M_PI_4};
  ASSERT_QUATERNION_NEAR(res.rotation, quaternion);
}

TEST(GeometryTest, TestDecomposition2) {
  auto rotated = Matrix::MakeRotationZ(Radians{M_PI_4});
  auto scaled = Matrix::MakeScale({2.0, 3.0, 1.0});
  auto translated = Matrix::MakeTranslation({-200, 750, 20});

  auto result = (translated * rotated * scaled).Decompose();

  ASSERT_TRUE(result.has_value());

  MatrixDecomposition res = result.value();

  auto quaternion = Quaternion{{0.0, 0.0, 1.0}, M_PI_4};

  ASSERT_QUATERNION_NEAR(res.rotation, quaternion);

  ASSERT_FLOAT_EQ(res.translation.x, -200);
  ASSERT_FLOAT_EQ(res.translation.y, 750);
  ASSERT_FLOAT_EQ(res.translation.z, 20);

  ASSERT_FLOAT_EQ(res.scale.x, 2);
  ASSERT_FLOAT_EQ(res.scale.y, 3);
  ASSERT_FLOAT_EQ(res.scale.z, 1);
}

TEST(GeometryTest, TestRecomposition) {
  /*
   *  Decomposition.
   */
  auto rotated = Matrix::MakeRotationZ(Radians{M_PI_4});

  auto result = rotated.Decompose();

  ASSERT_TRUE(result.has_value());

  MatrixDecomposition res = result.value();

  auto quaternion = Quaternion{{0.0, 0.0, 1.0}, M_PI_4};

  ASSERT_QUATERNION_NEAR(res.rotation, quaternion);

  /*
   *  Recomposition.
   */
  ASSERT_MATRIX_NEAR(rotated, Matrix{res});
}

TEST(GeometryTest, TestRecomposition2) {
  auto matrix = Matrix::MakeTranslation({100, 100, 100}) *
                Matrix::MakeRotationZ(Radians{M_PI_4}) *
                Matrix::MakeScale({2.0, 2.0, 2.0});

  auto result = matrix.Decompose();

  ASSERT_TRUE(result.has_value());

  ASSERT_MATRIX_NEAR(matrix, Matrix{result.value()});
}

TEST(GeometryTest, QuaternionLerp) {
  auto q1 = Quaternion{{0.0, 0.0, 1.0}, 0.0};
  auto q2 = Quaternion{{0.0, 0.0, 1.0}, M_PI_4};

  auto q3 = q1.Slerp(q2, 0.5);

  auto expected = Quaternion{{0.0, 0.0, 1.0}, M_PI_4 / 2.0};

  ASSERT_QUATERNION_NEAR(q3, expected);
}

TEST(GeometryTest, SimplePath) {
  Path path;

  path.AddLinearComponent({0, 0}, {100, 100})
      .AddQuadraticComponent({100, 100}, {200, 200}, {300, 300})
      .AddCubicComponent({300, 300}, {400, 400}, {500, 500}, {600, 600});

  ASSERT_EQ(path.GetComponentCount(), 4u);

  path.EnumerateComponents(
      [](size_t index, const LinearPathComponent& linear) {
        Point p1(0, 0);
        Point p2(100, 100);
        ASSERT_EQ(index, 1u);
        ASSERT_EQ(linear.p1, p1);
        ASSERT_EQ(linear.p2, p2);
      },
      [](size_t index, const QuadraticPathComponent& quad) {
        Point p1(100, 100);
        Point cp(200, 200);
        Point p2(300, 300);
        ASSERT_EQ(index, 2u);
        ASSERT_EQ(quad.p1, p1);
        ASSERT_EQ(quad.cp, cp);
        ASSERT_EQ(quad.p2, p2);
      },
      [](size_t index, const CubicPathComponent& cubic) {
        Point p1(300, 300);
        Point cp1(400, 400);
        Point cp2(500, 500);
        Point p2(600, 600);
        ASSERT_EQ(index, 3u);
        ASSERT_EQ(cubic.p1, p1);
        ASSERT_EQ(cubic.cp1, cp1);
        ASSERT_EQ(cubic.cp2, cp2);
        ASSERT_EQ(cubic.p2, p2);
      },
      [](size_t index, const ContourComponent& contour) {
        Point p1(0, 0);
        ASSERT_EQ(index, 0u);
        ASSERT_EQ(contour.destination, p1);
        ASSERT_FALSE(contour.is_closed);
      });
}

TEST(GeometryTest, BoundingBoxCubic) {
  Path path;
  path.AddCubicComponent({120, 160}, {25, 200}, {220, 260}, {220, 40});
  auto box = path.GetBoundingBox();
  Rect expected(93.9101, 40, 126.09, 158.862);
  ASSERT_TRUE(box.has_value());
  ASSERT_RECT_NEAR(box.value(), expected);
}

TEST(GeometryTest, BoundingBoxOfCompositePathIsCorrect) {
  PathBuilder builder;
  builder.AddRoundedRect({{10, 10}, {300, 300}}, {50, 50, 50, 50});
  auto path = builder.TakePath();
  auto actual = path.GetBoundingBox();
  Rect expected(10, 10, 300, 300);
  ASSERT_TRUE(actual.has_value());
  ASSERT_RECT_NEAR(actual.value(), expected);
}

TEST(GeometryTest, CanGenerateMipCounts) {
  ASSERT_EQ((Size{128, 128}.MipCount()), 7u);
  ASSERT_EQ((Size{128, 256}.MipCount()), 8u);
  ASSERT_EQ((Size{128, 130}.MipCount()), 8u);
  ASSERT_EQ((Size{128, 257}.MipCount()), 9u);
  ASSERT_EQ((Size{257, 128}.MipCount()), 9u);
  ASSERT_EQ((Size{128, 0}.MipCount()), 1u);
  ASSERT_EQ((Size{128, -25}.MipCount()), 1u);
  ASSERT_EQ((Size{-128, 25}.MipCount()), 1u);
}

TEST(GeometryTest, CanConvertTTypesExplicitly) {
  {
    Point p1(1.0, 2.0);
    IPoint p2 = static_cast<IPoint>(p1);
    ASSERT_EQ(p2.x, 1u);
    ASSERT_EQ(p2.y, 2u);
  }

  {
    Size s1(1.0, 2.0);
    ISize s2 = static_cast<ISize>(s1);
    ASSERT_EQ(s2.width, 1u);
    ASSERT_EQ(s2.height, 2u);
  }

  {
    Size s1(1.0, 2.0);
    Point p1 = static_cast<Point>(s1);
    ASSERT_EQ(p1.x, 1u);
    ASSERT_EQ(p1.y, 2u);
  }

  {
    Rect r1(1.0, 2.0, 3.0, 4.0);
    IRect r2 = static_cast<IRect>(r1);
    ASSERT_EQ(r2.origin.x, 1u);
    ASSERT_EQ(r2.origin.y, 2u);
    ASSERT_EQ(r2.size.width, 3u);
    ASSERT_EQ(r2.size.height, 4u);
  }
}

TEST(GeometryTest, CanPerformAlgebraicPointOps) {
  {
    IPoint p1(1, 2);
    IPoint p2 = p1 + IPoint(1, 2);
    ASSERT_EQ(p2.x, 2u);
    ASSERT_EQ(p2.y, 4u);
  }

  {
    IPoint p1(3, 6);
    IPoint p2 = p1 - IPoint(1, 2);
    ASSERT_EQ(p2.x, 2u);
    ASSERT_EQ(p2.y, 4u);
  }

  {
    IPoint p1(1, 2);
    IPoint p2 = p1 * IPoint(2, 3);
    ASSERT_EQ(p2.x, 2u);
    ASSERT_EQ(p2.y, 6u);
  }

  {
    IPoint p1(2, 6);
    IPoint p2 = p1 / IPoint(2, 3);
    ASSERT_EQ(p2.x, 1u);
    ASSERT_EQ(p2.y, 2u);
  }
}

TEST(GeometryTest, CanPerformAlgebraicPointOpsWithArithmeticTypes) {
  // LHS
  {
    IPoint p1(1, 2);
    IPoint p2 = p1 * 2.0f;
    ASSERT_EQ(p2.x, 2u);
    ASSERT_EQ(p2.y, 4u);
  }

  {
    IPoint p1(2, 6);
    IPoint p2 = p1 / 2.0f;
    ASSERT_EQ(p2.x, 1u);
    ASSERT_EQ(p2.y, 3u);
  }

  // RHS
  {
    IPoint p1(1, 2);
    IPoint p2 = 2.0f * p1;
    ASSERT_EQ(p2.x, 2u);
    ASSERT_EQ(p2.y, 4u);
  }

  {
    IPoint p1(2, 6);
    IPoint p2 = 12.0f / p1;
    ASSERT_EQ(p2.x, 6u);
    ASSERT_EQ(p2.y, 2u);
  }
}

TEST(GeometryTest, PointIntegerCoercesToFloat) {
  // Integer on LHS, float on RHS
  {
    IPoint p1(1, 2);
    Point p2 = p1 + Point(1, 2);
    ASSERT_FLOAT_EQ(p2.x, 2u);
    ASSERT_FLOAT_EQ(p2.y, 4u);
  }

  {
    IPoint p1(3, 6);
    Point p2 = p1 - Point(1, 2);
    ASSERT_FLOAT_EQ(p2.x, 2u);
    ASSERT_FLOAT_EQ(p2.y, 4u);
  }

  {
    IPoint p1(1, 2);
    Point p2 = p1 * Point(2, 3);
    ASSERT_FLOAT_EQ(p2.x, 2u);
    ASSERT_FLOAT_EQ(p2.y, 6u);
  }

  {
    IPoint p1(2, 6);
    Point p2 = p1 / Point(2, 3);
    ASSERT_FLOAT_EQ(p2.x, 1u);
    ASSERT_FLOAT_EQ(p2.y, 2u);
  }

  // Float on LHS, integer on RHS
  {
    Point p1(1, 2);
    Point p2 = p1 + IPoint(1, 2);
    ASSERT_FLOAT_EQ(p2.x, 2u);
    ASSERT_FLOAT_EQ(p2.y, 4u);
  }

  {
    Point p1(3, 6);
    Point p2 = p1 - IPoint(1, 2);
    ASSERT_FLOAT_EQ(p2.x, 2u);
    ASSERT_FLOAT_EQ(p2.y, 4u);
  }

  {
    Point p1(1, 2);
    Point p2 = p1 * IPoint(2, 3);
    ASSERT_FLOAT_EQ(p2.x, 2u);
    ASSERT_FLOAT_EQ(p2.y, 6u);
  }

  {
    Point p1(2, 6);
    Point p2 = p1 / IPoint(2, 3);
    ASSERT_FLOAT_EQ(p2.x, 1u);
    ASSERT_FLOAT_EQ(p2.y, 2u);
  }
}

TEST(GeometryTest, SizeCoercesToPoint) {
  // Point on LHS, Size on RHS
  {
    IPoint p1(1, 2);
    IPoint p2 = p1 + ISize(1, 2);
    ASSERT_EQ(p2.x, 2u);
    ASSERT_EQ(p2.y, 4u);
  }

  {
    IPoint p1(3, 6);
    IPoint p2 = p1 - ISize(1, 2);
    ASSERT_EQ(p2.x, 2u);
    ASSERT_EQ(p2.y, 4u);
  }

  {
    IPoint p1(1, 2);
    IPoint p2 = p1 * ISize(2, 3);
    ASSERT_EQ(p2.x, 2u);
    ASSERT_EQ(p2.y, 6u);
  }

  {
    IPoint p1(2, 6);
    IPoint p2 = p1 / ISize(2, 3);
    ASSERT_EQ(p2.x, 1u);
    ASSERT_EQ(p2.y, 2u);
  }

  // Size on LHS, Point on RHS
  {
    ISize p1(1, 2);
    IPoint p2 = p1 + IPoint(1, 2);
    ASSERT_EQ(p2.x, 2u);
    ASSERT_EQ(p2.y, 4u);
  }

  {
    ISize p1(3, 6);
    IPoint p2 = p1 - IPoint(1, 2);
    ASSERT_EQ(p2.x, 2u);
    ASSERT_EQ(p2.y, 4u);
  }

  {
    ISize p1(1, 2);
    IPoint p2 = p1 * IPoint(2, 3);
    ASSERT_EQ(p2.x, 2u);
    ASSERT_EQ(p2.y, 6u);
  }

  {
    ISize p1(2, 6);
    IPoint p2 = p1 / IPoint(2, 3);
    ASSERT_EQ(p2.x, 1u);
    ASSERT_EQ(p2.y, 2u);
  }
}

TEST(GeometryTest, CanUsePointAssignmentOperators) {
  // Point on RHS
  {
    IPoint p(1, 2);
    p += IPoint(1, 2);
    ASSERT_EQ(p.x, 2u);
    ASSERT_EQ(p.y, 4u);
  }

  {
    IPoint p(3, 6);
    p -= IPoint(1, 2);
    ASSERT_EQ(p.x, 2u);
    ASSERT_EQ(p.y, 4u);
  }

  {
    IPoint p(1, 2);
    p *= IPoint(2, 3);
    ASSERT_EQ(p.x, 2u);
    ASSERT_EQ(p.y, 6u);
  }

  {
    IPoint p(2, 6);
    p /= IPoint(2, 3);
    ASSERT_EQ(p.x, 1u);
    ASSERT_EQ(p.y, 2u);
  }

  // Size on RHS
  {
    IPoint p(1, 2);
    p += ISize(1, 2);
    ASSERT_EQ(p.x, 2u);
    ASSERT_EQ(p.y, 4u);
  }

  {
    IPoint p(3, 6);
    p -= ISize(1, 2);
    ASSERT_EQ(p.x, 2u);
    ASSERT_EQ(p.y, 4u);
  }

  {
    IPoint p(1, 2);
    p *= ISize(2, 3);
    ASSERT_EQ(p.x, 2u);
    ASSERT_EQ(p.y, 6u);
  }

  {
    IPoint p(2, 6);
    p /= ISize(2, 3);
    ASSERT_EQ(p.x, 1u);
    ASSERT_EQ(p.y, 2u);
  }
}

TEST(GeometryTest, PointDotProduct) {
  {
    Point p(1, 0);
    Scalar s = p.Dot(Point(-1, 0));
    ASSERT_FLOAT_EQ(s, -1);
  }

  {
    Point p(0, -1);
    Scalar s = p.Dot(Point(-1, 0));
    ASSERT_FLOAT_EQ(s, 0);
  }

  {
    Point p(1, 2);
    Scalar s = p.Dot(Point(3, -4));
    ASSERT_FLOAT_EQ(s, -5);
  }
}

TEST(GeometryTest, PointCrossProduct) {
  {
    Point p(1, 0);
    Scalar s = p.Cross(Point(-1, 0));
    ASSERT_FLOAT_EQ(s, 0);
  }

  {
    Point p(0, -1);
    Scalar s = p.Cross(Point(-1, 0));
    ASSERT_FLOAT_EQ(s, -1);
  }

  {
    Point p(1, 2);
    Scalar s = p.Cross(Point(3, -4));
    ASSERT_FLOAT_EQ(s, -10);
  }
}

TEST(GeometryTest, PointReflect) {
  {
    Point axis = Point(0, 1);
    Point a(2, 3);
    auto reflected = a.Reflect(axis);
    auto expected = Point(2, -3);
    ASSERT_POINT_NEAR(reflected, expected);
  }

  {
    Point axis = Point(1, 1).Normalize();
    Point a(1, 0);
    auto reflected = a.Reflect(axis);
    auto expected = Point(0, -1);
    ASSERT_POINT_NEAR(reflected, expected);
  }

  {
    Point axis = Point(1, 1).Normalize();
    Point a(-1, -1);
    auto reflected = a.Reflect(axis);
    ASSERT_POINT_NEAR(reflected, -a);
  }
}

TEST(GeometryTest, CanConvertBetweenDegressAndRadians) {
  {
    auto deg = Degrees{90.0};
    Radians rad = deg;
    ASSERT_FLOAT_EQ(rad.radians, kPiOver2);
  }
}

TEST(GeometryTest, RectUnion) {
  {
    Rect a(100, 100, 100, 100);
    Rect b(0, 0, 0, 0);
    auto u = a.Union(b);
    auto expected = Rect(0, 0, 200, 200);
    ASSERT_RECT_NEAR(u, expected);
  }

  {
    Rect a(100, 100, 100, 100);
    Rect b(10, 10, 0, 0);
    auto u = a.Union(b);
    auto expected = Rect(10, 10, 190, 190);
    ASSERT_RECT_NEAR(u, expected);
  }

  {
    Rect a(0, 0, 100, 100);
    Rect b(10, 10, 100, 100);
    auto u = a.Union(b);
    auto expected = Rect(0, 0, 110, 110);
    ASSERT_RECT_NEAR(u, expected);
  }

  {
    Rect a(0, 0, 100, 100);
    Rect b(100, 100, 100, 100);
    auto u = a.Union(b);
    auto expected = Rect(0, 0, 200, 200);
    ASSERT_RECT_NEAR(u, expected);
  }
}

TEST(GeometryTest, RectIntersection) {
  {
    Rect a(100, 100, 100, 100);
    Rect b(0, 0, 0, 0);

    auto u = a.Intersection(b);
    ASSERT_FALSE(u.has_value());
  }

  {
    Rect a(100, 100, 100, 100);
    Rect b(10, 10, 0, 0);
    auto u = a.Intersection(b);
    ASSERT_FALSE(u.has_value());
  }

  {
    Rect a(0, 0, 100, 100);
    Rect b(10, 10, 100, 100);
    auto u = a.Intersection(b);
    ASSERT_TRUE(u.has_value());
    auto expected = Rect(10, 10, 90, 90);
    ASSERT_RECT_NEAR(u.value(), expected);
  }

  {
    Rect a(0, 0, 100, 100);
    Rect b(100, 100, 100, 100);
    auto u = a.Intersection(b);
    ASSERT_FALSE(u.has_value());
  }
}

TEST(GeometryTest, RectContainsPoint) {
  {
    // Origin is inclusive
    Rect r(100, 100, 100, 100);
    Point p(100, 100);
    ASSERT_TRUE(r.Contains(p));
  }
  {
    // Size is exclusive
    Rect r(100, 100, 100, 100);
    Point p(200, 200);
    ASSERT_FALSE(r.Contains(p));
  }
  {
    Rect r(100, 100, 100, 100);
    Point p(99, 99);
    ASSERT_FALSE(r.Contains(p));
  }
  {
    Rect r(100, 100, 100, 100);
    Point p(199, 199);
    ASSERT_TRUE(r.Contains(p));
  }
}

TEST(GeometryTest, RectContainsRect) {
  {
    Rect a(100, 100, 100, 100);
    ASSERT_TRUE(a.Contains(a));
  }
  {
    Rect a(100, 100, 100, 100);
    Rect b(0, 0, 0, 0);
    ASSERT_FALSE(a.Contains(b));
  }
  {
    Rect a(100, 100, 100, 100);
    Rect b(150, 150, 20, 20);
    ASSERT_TRUE(a.Contains(b));
  }
  {
    Rect a(100, 100, 100, 100);
    Rect b(150, 150, 100, 100);
    ASSERT_FALSE(a.Contains(b));
  }
  {
    Rect a(100, 100, 100, 100);
    Rect b(50, 50, 100, 100);
    ASSERT_FALSE(a.Contains(b));
  }
  {
    Rect a(100, 100, 100, 100);
    Rect b(0, 0, 300, 300);
    ASSERT_FALSE(a.Contains(b));
  }
}

TEST(GeometryTest, CubicPathComponentPolylineDoesNotIncludePointOne) {
  CubicPathComponent component({10, 10}, {20, 35}, {35, 20}, {40, 40});
  SmoothingApproximation approximation;
  auto polyline = component.CreatePolyline(approximation);
  ASSERT_NE(polyline.front().x, 10);
  ASSERT_NE(polyline.front().y, 10);
  ASSERT_EQ(polyline.back().x, 40);
  ASSERT_EQ(polyline.back().y, 40);
}

TEST(GeometryTest, PathCreatePolyLineDoesNotDuplicatePoints) {
  Path path;
  path.AddContourComponent({10, 10});
  path.AddLinearComponent({10, 10}, {20, 20});
  path.AddLinearComponent({20, 20}, {30, 30});
  path.AddContourComponent({40, 40});
  path.AddLinearComponent({40, 40}, {50, 50});

  auto polyline = path.CreatePolyline();

  ASSERT_EQ(polyline.contours.size(), 2u);
  ASSERT_EQ(polyline.points.size(), 5u);
  ASSERT_EQ(polyline.points[0].x, 10);
  ASSERT_EQ(polyline.points[1].x, 20);
  ASSERT_EQ(polyline.points[2].x, 30);
  ASSERT_EQ(polyline.points[3].x, 40);
  ASSERT_EQ(polyline.points[4].x, 50);
}

TEST(GeometryTest, PathBuilderSetsCorrectContourPropertiesForAddCommands) {
  // Closed shapes.
  {
    Path path = PathBuilder{}.AddCircle({100, 100}, 50).TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(100, 50));
    ASSERT_TRUE(contour.is_closed);
  }

  {
    Path path = PathBuilder{}.AddOval(Rect(100, 100, 100, 100)).TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(150, 100));
    ASSERT_TRUE(contour.is_closed);
  }

  {
    Path path = PathBuilder{}.AddRect(Rect(100, 100, 100, 100)).TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(100, 100));
    ASSERT_TRUE(contour.is_closed);
  }

  {
    Path path =
        PathBuilder{}.AddRoundedRect(Rect(100, 100, 100, 100), 10).TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(110, 100));
    ASSERT_TRUE(contour.is_closed);
  }

  // Open shapes.
  {
    Point p(100, 100);
    Path path = PathBuilder{}.AddLine(p, {200, 100}).TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, p);
    ASSERT_FALSE(contour.is_closed);
  }

  {
    Path path =
        PathBuilder{}
            .AddCubicCurve({100, 100}, {100, 50}, {100, 150}, {200, 100})
            .TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(100, 100));
    ASSERT_FALSE(contour.is_closed);
  }

  {
    Path path = PathBuilder{}
                    .AddQuadraticCurve({100, 100}, {100, 50}, {200, 100})
                    .TakePath();
    ContourComponent contour;
    path.GetContourComponentAtIndex(0, contour);
    ASSERT_POINT_NEAR(contour.destination, Point(100, 100));
    ASSERT_FALSE(contour.is_closed);
  }
}

TEST(GeometryTest, PathCreatePolylineGeneratesCorrectContourData) {
  Path::Polyline polyline = PathBuilder{}
                                .AddLine({100, 100}, {200, 100})
                                .MoveTo({100, 200})
                                .LineTo({150, 250})
                                .LineTo({200, 200})
                                .Close()
                                .TakePath()
                                .CreatePolyline();
  ASSERT_EQ(polyline.points.size(), 6u);
  ASSERT_EQ(polyline.contours.size(), 2u);
  ASSERT_EQ(polyline.contours[0].is_closed, false);
  ASSERT_EQ(polyline.contours[0].start_index, 0u);
  ASSERT_EQ(polyline.contours[1].is_closed, true);
  ASSERT_EQ(polyline.contours[1].start_index, 2u);
}

TEST(GeometryTest, PolylineGetContourPointBoundsReturnsCorrectRanges) {
  Path::Polyline polyline = PathBuilder{}
                                .AddLine({100, 100}, {200, 100})
                                .MoveTo({100, 200})
                                .LineTo({150, 250})
                                .LineTo({200, 200})
                                .Close()
                                .TakePath()
                                .CreatePolyline();
  size_t a1, a2, b1, b2;
  std::tie(a1, a2) = polyline.GetContourPointBounds(0);
  std::tie(b1, b2) = polyline.GetContourPointBounds(1);
  ASSERT_EQ(a1, 0u);
  ASSERT_EQ(a2, 2u);
  ASSERT_EQ(b1, 2u);
  ASSERT_EQ(b2, 6u);
}

}  // namespace testing
}  // namespace impeller
