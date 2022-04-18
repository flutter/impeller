// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gtest/gtest.h"
#include "third_party/imgui/imgui.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkPathBuilder.h"

#include "flutter/display_list/display_list_builder.h"
#include "flutter/testing/testing.h"
#include "impeller/display_list/display_list_image_impeller.h"
#include "impeller/display_list/display_list_playground.h"
#include "impeller/geometry/point.h"
#include "impeller/playground/widgets.h"

namespace impeller {
namespace testing {

using DisplayListTest = DisplayListPlayground;
INSTANTIATE_PLAYGROUND_SUITE(DisplayListTest);

TEST_P(DisplayListTest, CanDrawRect) {
  flutter::DisplayListBuilder builder;
  builder.setColor(SK_ColorBLUE);
  builder.drawRect(SkRect::MakeXYWH(10, 10, 100, 100));
  ASSERT_TRUE(OpenPlaygroundHere(builder.Build()));
}

TEST_P(DisplayListTest, CanDrawTextBlob) {
  flutter::DisplayListBuilder builder;
  builder.setColor(SK_ColorBLUE);
  builder.drawTextBlob(SkTextBlob::MakeFromString("Hello", CreateTestFont()),
                       100, 100);
  ASSERT_TRUE(OpenPlaygroundHere(builder.Build()));
}

TEST_P(DisplayListTest, CanDrawImage) {
  auto texture = CreateTextureForFixture("embarcadero.jpg");
  flutter::DisplayListBuilder builder;
  builder.drawImage(DlImageImpeller::Make(texture), SkPoint::Make(100, 100),
                    SkSamplingOptions{}, true);
  ASSERT_TRUE(OpenPlaygroundHere(builder.Build()));
}

TEST_P(DisplayListTest, CanDrawCapsAndJoins) {
  flutter::DisplayListBuilder builder;

  builder.setStyle(SkPaint::Style::kStroke_Style);
  builder.setStrokeWidth(30);
  builder.setColor(SK_ColorRED);

  auto path =
      SkPathBuilder{}.moveTo(-50, 0).lineTo(0, -50).lineTo(50, 0).snapshot();

  builder.translate(100, 100);
  {
    builder.setStrokeCap(SkPaint::Cap::kButt_Cap);
    builder.setStrokeJoin(SkPaint::Join::kMiter_Join);
    builder.setStrokeMiter(4);
    builder.drawPath(path);
  }

  {
    builder.save();
    builder.translate(0, 100);
    // The joint in the path is 45 degrees. A miter length of 1 convert to a
    // bevel in this case.
    builder.setStrokeMiter(1);
    builder.drawPath(path);
    builder.restore();
  }

  builder.translate(150, 0);
  {
    builder.setStrokeCap(SkPaint::Cap::kSquare_Cap);
    builder.setStrokeJoin(SkPaint::Join::kBevel_Join);
    builder.drawPath(path);
  }

  builder.translate(150, 0);
  {
    builder.setStrokeCap(SkPaint::Cap::kRound_Cap);
    builder.setStrokeJoin(SkPaint::Join::kRound_Join);
    builder.drawPath(path);
  }

  ASSERT_TRUE(OpenPlaygroundHere(builder.Build()));
}

TEST_P(DisplayListTest, CanDrawArc) {
  bool first_frame = true;
  auto callback = [&]() {
    if (first_frame) {
      first_frame = false;
      ImGui::SetNextWindowSize({400, 100});
      ImGui::SetNextWindowPos({300, 550});
    }

    static float start_angle = 45;
    static float sweep_angle = 270;
    static bool use_center = true;

    ImGui::Begin("Controls");
    ImGui::SliderFloat("Start angle", &start_angle, -360, 360);
    ImGui::SliderFloat("Sweep angle", &sweep_angle, -360, 360);
    ImGui::Checkbox("Use center", &use_center);
    ImGui::End();

    auto [p1, p2] = IMPELLER_PLAYGROUND_LINE(
        Point(200, 200), Point(400, 400), 20, Color::White(), Color::White());

    flutter::DisplayListBuilder builder;
    builder.setStyle(SkPaint::Style::kStroke_Style);
    builder.setStrokeCap(SkPaint::Cap::kRound_Cap);
    builder.setStrokeJoin(SkPaint::Join::kMiter_Join);
    builder.setStrokeMiter(10);
    auto rect = SkRect::MakeLTRB(p1.x, p1.y, p2.x, p2.y);
    builder.setColor(SK_ColorGREEN);
    builder.setStrokeWidth(2);
    builder.drawRect(rect);
    builder.setColor(SK_ColorRED);
    builder.setStrokeWidth(10);
    builder.drawArc(rect, start_angle, sweep_angle, use_center);

    return builder.Build();
  };
  ASSERT_TRUE(OpenPlaygroundHere(callback));
}

TEST_P(DisplayListTest, StrokedPathsDrawCorrectly) {
  flutter::DisplayListBuilder builder;
  builder.setColor(SK_ColorRED);
  builder.setStyle(SkPaint::Style::kStroke_Style);
  builder.setStrokeWidth(10);

  // Rectangle
  builder.translate(100, 100);
  builder.drawRect(SkRect::MakeSize({100, 100}));

  // Rounded rectangle
  builder.translate(150, 0);
  builder.drawRRect(SkRRect::MakeRectXY(SkRect::MakeSize({100, 50}), 10, 10));

  // Double rounded rectangle
  builder.translate(150, 0);
  builder.drawDRRect(
      SkRRect::MakeRectXY(SkRect::MakeSize({100, 50}), 10, 10),
      SkRRect::MakeRectXY(SkRect::MakeXYWH(10, 10, 80, 30), 10, 10));

  // Contour with duplicate join points
  {
    builder.translate(150, 0);
    SkPath path;
    path.lineTo({100, 0});
    path.lineTo({100, 0});
    path.lineTo({100, 100});
    builder.drawPath(path);
  }

  // Contour with duplicate end points
  {
    builder.setStrokeCap(SkPaint::Cap::kRound_Cap);
    builder.translate(150, 0);
    SkPath path;
    path.moveTo(0, 0);
    path.lineTo({0, 0});
    path.lineTo({50, 50});
    path.lineTo({100, 0});
    path.lineTo({100, 0});
    builder.drawPath(path);
  }

  ASSERT_TRUE(OpenPlaygroundHere(builder.Build()));
}

}  // namespace testing
}  // namespace impeller
