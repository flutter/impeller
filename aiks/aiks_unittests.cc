// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>

#include "flutter/testing/testing.h"
#include "impeller/aiks/aiks_playground.h"
#include "impeller/aiks/canvas.h"
#include "impeller/aiks/image.h"
#include "impeller/geometry/geometry_unittests.h"
#include "impeller/geometry/path_builder.h"
#include "impeller/playground/widgets.h"
#include "impeller/typographer/backends/skia/text_frame_skia.h"
#include "impeller/typographer/backends/skia/text_render_context_skia.h"
#include "third_party/skia/include/core/SkData.h"

namespace impeller {
namespace testing {

using AiksTest = AiksPlayground;
INSTANTIATE_PLAYGROUND_SUITE(AiksTest);

TEST_P(AiksTest, CanvasCTMCanBeUpdated) {
  Canvas canvas;
  Matrix identity;
  ASSERT_MATRIX_NEAR(canvas.GetCurrentTransformation(), identity);
  canvas.Translate(Size{100, 100});
  ASSERT_MATRIX_NEAR(canvas.GetCurrentTransformation(),
                     Matrix::MakeTranslation({100.0, 100.0, 0.0}));
}

TEST_P(AiksTest, CanvasCanPushPopCTM) {
  Canvas canvas;
  ASSERT_EQ(canvas.GetSaveCount(), 1u);
  ASSERT_EQ(canvas.Restore(), false);

  canvas.Translate(Size{100, 100});
  canvas.Save();
  ASSERT_EQ(canvas.GetSaveCount(), 2u);
  ASSERT_MATRIX_NEAR(canvas.GetCurrentTransformation(),
                     Matrix::MakeTranslation({100.0, 100.0, 0.0}));
  ASSERT_TRUE(canvas.Restore());
  ASSERT_EQ(canvas.GetSaveCount(), 1u);
  ASSERT_MATRIX_NEAR(canvas.GetCurrentTransformation(),
                     Matrix::MakeTranslation({100.0, 100.0, 0.0}));
}

TEST_P(AiksTest, CanRenderColoredRect) {
  Canvas canvas;
  Paint paint;
  paint.color = Color::Red();
  canvas.DrawPath(PathBuilder{}
                      .AddRect(Rect::MakeXYWH(100.0, 100.0, 100.0, 100.0))
                      .TakePath(),
                  paint);
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderImage) {
  Canvas canvas;
  Paint paint;
  auto image = std::make_shared<Image>(CreateTextureForFixture("kalimba.jpg"));
  paint.color = Color::Red();
  canvas.DrawImage(image, Point::MakeXY(100.0, 100.0), paint);
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderImageRect) {
  Canvas canvas;
  Paint paint;
  auto image = std::make_shared<Image>(CreateTextureForFixture("kalimba.jpg"));
  auto source_rect = Rect::MakeSize(Size(image->GetSize()));

  // Render the bottom right quarter of the source image in a stretched rect.
  source_rect.size.width /= 2;
  source_rect.size.height /= 2;
  source_rect.origin.x += source_rect.size.width;
  source_rect.origin.y += source_rect.size.height;
  canvas.DrawImageRect(image, source_rect, Rect::MakeXYWH(100, 100, 600, 600),
                       paint);
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderStrokes) {
  Canvas canvas;
  Paint paint;
  paint.color = Color::Red();
  paint.stroke_width = 20.0;
  paint.style = Paint::Style::kStroke;
  canvas.DrawPath(PathBuilder{}.AddLine({200, 100}, {800, 100}).TakePath(),
                  paint);
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderCurvedStrokes) {
  Canvas canvas;
  Paint paint;
  paint.color = Color::Red();
  paint.stroke_width = 25.0;
  paint.style = Paint::Style::kStroke;
  canvas.DrawPath(PathBuilder{}.AddCircle({500, 500}, 250).TakePath(), paint);
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderClips) {
  Canvas canvas;
  Paint paint;
  paint.color = Color::Fuchsia();
  canvas.ClipPath(
      PathBuilder{}.AddRect(Rect::MakeXYWH(0, 0, 500, 500)).TakePath());
  canvas.DrawPath(PathBuilder{}.AddCircle({500, 500}, 250).TakePath(), paint);
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderNestedClips) {
  Canvas canvas;
  Paint paint;
  paint.color = Color::Fuchsia();
  canvas.Save();
  canvas.ClipPath(PathBuilder{}.AddCircle({200, 400}, 300).TakePath());
  canvas.Restore();
  canvas.ClipPath(PathBuilder{}.AddCircle({600, 400}, 300).TakePath());
  canvas.ClipPath(PathBuilder{}.AddCircle({400, 600}, 300).TakePath());
  canvas.DrawRect(Rect::MakeXYWH(200, 200, 400, 400), paint);
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderDifferenceClips) {
  Paint paint;
  Canvas canvas;
  canvas.Translate({400, 400});

  // Limit drawing to face circle with a clip.
  canvas.ClipPath(PathBuilder{}.AddCircle(Point(), 200).TakePath());
  canvas.Save();

  // Cut away eyes/mouth using difference clips.
  canvas.ClipPath(PathBuilder{}.AddCircle({-100, -50}, 30).TakePath(),
                  Entity::ClipOperation::kDifference);
  canvas.ClipPath(PathBuilder{}.AddCircle({100, -50}, 30).TakePath(),
                  Entity::ClipOperation::kDifference);
  canvas.ClipPath(PathBuilder{}
                      .AddQuadraticCurve({-100, 50}, {0, 150}, {100, 50})
                      .TakePath(),
                  Entity::ClipOperation::kDifference);

  // Draw a huge yellow rectangle to prove the clipping works.
  paint.color = Color::Yellow();
  canvas.DrawRect(Rect::MakeXYWH(-1000, -1000, 2000, 2000), paint);

  // Remove the difference clips and draw hair that partially covers the eyes.
  canvas.Restore();
  paint.color = Color::Maroon();
  canvas.DrawPath(PathBuilder{}
                      .MoveTo({200, -200})
                      .HorizontalLineTo(-200)
                      .VerticalLineTo(-40)
                      .CubicCurveTo({0, -40}, {0, -80}, {200, -80})
                      .TakePath(),
                  paint);

  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, ClipsUseCurrentTransform) {
  std::array<Color, 5> colors = {Color::White(), Color::Black(),
                                 Color::SkyBlue(), Color::Red(),
                                 Color::Yellow()};
  Canvas canvas;
  Paint paint;

  canvas.Translate(Vector3(300, 300));
  for (int i = 0; i < 15; i++) {
    canvas.Scale(Vector3(0.8, 0.8));

    paint.color = colors[i % colors.size()];
    canvas.ClipPath(PathBuilder{}.AddCircle({0, 0}, 300).TakePath());
    canvas.DrawRect(Rect(-300, -300, 600, 600), paint);
  }
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanSaveLayerStandalone) {
  Canvas canvas;

  Paint red;
  red.color = Color::Red();

  Paint alpha;
  alpha.color = Color::Red().WithAlpha(0.5);

  canvas.SaveLayer(alpha);

  canvas.DrawCircle({125, 125}, 125, red);

  canvas.Restore();

  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderGroupOpacity) {
  Canvas canvas;

  Paint red;
  red.color = Color::Red();
  Paint green;
  green.color = Color::Green().WithAlpha(0.5);
  Paint blue;
  blue.color = Color::Blue();

  Paint alpha;
  alpha.color = Color::Red().WithAlpha(0.5);

  canvas.SaveLayer(alpha);

  canvas.DrawRect({000, 000, 100, 100}, red);
  canvas.DrawRect({020, 020, 100, 100}, green);
  canvas.DrawRect({040, 040, 100, 100}, blue);

  canvas.Restore();

  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanPerformFullScreenMSAA) {
  Canvas canvas;

  Paint red;
  red.color = Color::Red();

  canvas.DrawCircle({250, 250}, 125, red);

  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanPerformSkew) {
  Canvas canvas;

  Paint red;
  red.color = Color::Red();

  canvas.Skew(2, 5);
  canvas.DrawRect(Rect::MakeXYWH(0, 0, 100, 100), red);

  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanPerformSaveLayerWithBounds) {
  Canvas canvas;

  Paint red;
  red.color = Color::Red();

  Paint green;
  green.color = Color::Green();

  Paint blue;
  blue.color = Color::Blue();

  Paint save;
  save.color = Color::Black();

  canvas.SaveLayer(save, Rect{0, 0, 50, 50});

  canvas.DrawRect({0, 0, 100, 100}, red);
  canvas.DrawRect({10, 10, 100, 100}, green);
  canvas.DrawRect({20, 20, 100, 100}, blue);

  canvas.Restore();

  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest,
       CanPerformSaveLayerWithBoundsAndLargerIntermediateIsNotAllocated) {
  Canvas canvas;

  Paint red;
  red.color = Color::Red();

  Paint green;
  green.color = Color::Green();

  Paint blue;
  blue.color = Color::Blue();

  Paint save;
  save.color = Color::Black().WithAlpha(0.5);

  canvas.SaveLayer(save, Rect{0, 0, 100000, 100000});

  canvas.DrawRect({0, 0, 100, 100}, red);
  canvas.DrawRect({10, 10, 100, 100}, green);
  canvas.DrawRect({20, 20, 100, 100}, blue);

  canvas.Restore();

  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderRoundedRectWithNonUniformRadii) {
  Canvas canvas;

  Paint paint;
  paint.color = Color::Red();

  PathBuilder::RoundingRadii radii;
  radii.top_left = {50, 25};
  radii.top_right = {25, 50};
  radii.bottom_right = {50, 25};
  radii.bottom_left = {25, 50};

  auto path =
      PathBuilder{}.AddRoundedRect(Rect{100, 100, 500, 500}, radii).TakePath();

  canvas.DrawPath(path, paint);

  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderDifferencePaths) {
  Canvas canvas;

  Paint paint;
  paint.color = Color::Red();

  PathBuilder builder;

  PathBuilder::RoundingRadii radii;
  radii.top_left = {50, 25};
  radii.top_right = {25, 50};
  radii.bottom_right = {50, 25};
  radii.bottom_left = {25, 50};

  builder.AddRoundedRect({100, 100, 200, 200}, radii);
  builder.AddCircle({200, 200}, 50);
  auto path = builder.TakePath(FillType::kOdd);

  canvas.DrawImage(
      std::make_shared<Image>(CreateTextureForFixture("boston.jpg")), {10, 10},
      Paint{});
  canvas.DrawPath(std::move(path), paint);

  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

static sk_sp<SkData> OpenFixtureAsSkData(const char* fixture_name) {
  auto mapping = flutter::testing::OpenFixtureAsMapping(fixture_name);
  if (!mapping) {
    return nullptr;
  }
  return SkData::MakeWithProc(
      mapping->GetMapping(), mapping->GetSize(),
      [](const void* ptr, void* context) {
        delete reinterpret_cast<fml::Mapping*>(context);
      },
      mapping.release());
}

bool RenderTextInCanvas(std::shared_ptr<Context> context,
                        Canvas& canvas,
                        const std::string& text,
                        const std::string& font_fixture,
                        Scalar font_size = 50.0) {
  Scalar baseline = 200.0;
  Point text_position = {100, baseline};

  // Draw the baseline.
  canvas.DrawRect({50, baseline, 900, 10},
                  Paint{.color = Color::Aqua().WithAlpha(0.25)});

  // Mark the point at which the text is drawn.
  canvas.DrawCircle(text_position, 5.0,
                    Paint{.color = Color::Red().WithAlpha(0.25)});

  // Construct the text blob.
  auto mapping = OpenFixtureAsSkData(font_fixture.c_str());
  if (!mapping) {
    return false;
  }
  SkFont sk_font(SkTypeface::MakeFromData(mapping), 50.0);
  auto blob = SkTextBlob::MakeFromString(text.c_str(), sk_font);
  if (!blob) {
    return false;
  }

  // Create the Impeller text frame and draw it at the designated baseline.
  auto frame = TextFrameFromTextBlob(blob);

  Paint text_paint;
  text_paint.color = Color::Yellow();
  canvas.DrawTextFrame(std::move(frame), text_position, text_paint);
  return true;
}

TEST_P(AiksTest, CanRenderTextFrame) {
  Canvas canvas;
  ASSERT_TRUE(RenderTextInCanvas(
      GetContext(), canvas, "the quick brown fox jumped over the lazy dog!.?",
      "Roboto-Regular.ttf"));
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderItalicizedText) {
  Canvas canvas;
  ASSERT_TRUE(RenderTextInCanvas(
      GetContext(), canvas, "the quick brown fox jumped over the lazy dog!.?",
      "HomemadeApple.ttf"));
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderEmojiTextFrame) {
  Canvas canvas;
  ASSERT_TRUE(RenderTextInCanvas(
      GetContext(), canvas,
      "😀 😃 😄 😁 😆 😅 😂 🤣 🥲 ☺️ 😊",
      "NotoColorEmoji.ttf"));
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanRenderTextInSaveLayer) {
  Canvas canvas;
  canvas.DrawPaint({.color = Color::White()});
  canvas.Translate({100, 100});
  canvas.Scale(Vector2{0.5, 0.5});

  // Blend the layer with the parent pass using kClear to expose the coverage.
  canvas.SaveLayer({.blend_mode = Entity::BlendMode::kClear});
  ASSERT_TRUE(RenderTextInCanvas(
      GetContext(), canvas, "the quick brown fox jumped over the lazy dog!.?",
      "Roboto-Regular.ttf"));
  canvas.Restore();

  // Render the text again over the cleared coverage rect.
  ASSERT_TRUE(RenderTextInCanvas(
      GetContext(), canvas, "the quick brown fox jumped over the lazy dog!.?",
      "Roboto-Regular.ttf"));

  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, CanDrawPaint) {
  Paint paint;
  paint.color = Color::MediumTurquoise();
  Canvas canvas;
  canvas.DrawPaint(paint);
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, PaintBlendModeIsRespected) {
  Paint paint;
  Canvas canvas;
  // Default is kSourceOver.
  paint.color = Color(1, 0, 0, 0.5);
  canvas.DrawCircle(Point(150, 200), 100, paint);
  paint.color = Color(0, 1, 0, 0.5);
  canvas.DrawCircle(Point(250, 200), 100, paint);

  paint.blend_mode = Entity::BlendMode::kPlus;
  paint.color = Color::Red();
  canvas.DrawCircle(Point(450, 250), 100, paint);
  paint.color = Color::Green();
  canvas.DrawCircle(Point(550, 250), 100, paint);
  paint.color = Color::Blue();
  canvas.DrawCircle(Point(500, 150), 100, paint);
  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

TEST_P(AiksTest, TransformMultipliesCorrectly) {
  Canvas canvas;
  ASSERT_MATRIX_NEAR(canvas.GetCurrentTransformation(), Matrix());

  // clang-format off
  canvas.Translate(Vector3(100, 200));
  ASSERT_MATRIX_NEAR(
    canvas.GetCurrentTransformation(),
    Matrix(  1,   0,   0,   0,
             0,   1,   0,   0,
             0,   0,   1,   0,
           100, 200,   0,   1));

  canvas.Rotate(Radians(kPiOver2));
  ASSERT_MATRIX_NEAR(
    canvas.GetCurrentTransformation(),
    Matrix(  0,   1,   0,   0,
            -1,   0,   0,   0,
             0,   0,   1,   0,
           100, 200,   0,   1));

  canvas.Scale(Vector3(2, 3));
  ASSERT_MATRIX_NEAR(
    canvas.GetCurrentTransformation(),
    Matrix(  0,   2,   0,   0,
            -3,   0,   0,   0,
             0,   0,   0,   0,
           100, 200,   0,   1));

  canvas.Translate(Vector3(100, 200));
  ASSERT_MATRIX_NEAR(
    canvas.GetCurrentTransformation(),
    Matrix(   0,   2,   0,   0,
             -3,   0,   0,   0,
              0,   0,   0,   0,
           -500, 400,   0,   1));
  // clang-format on
}

TEST_P(AiksTest, SolidStrokesRenderCorrectly) {
  // Compare with https://fiddle.skia.org/c/027392122bec8ac2b5d5de00a4b9bbe2
  bool first_frame = true;
  auto callback = [&](AiksContext& renderer, RenderPass& pass) {
    if (first_frame) {
      first_frame = false;
      ImGui::SetNextWindowSize({480, 100});
      ImGui::SetNextWindowPos({100, 550});
    }

    static Color color = Color::Black().WithAlpha(0.5);
    static float scale = 3;
    static bool add_circle_clip = true;

    ImGui::Begin("Controls");
    ImGui::ColorEdit4("Color", reinterpret_cast<float*>(&color));
    ImGui::SliderFloat("Scale", &scale, 0, 6);
    ImGui::Checkbox("Circle clip", &add_circle_clip);
    ImGui::End();

    Canvas canvas;
    Paint paint;

    paint.color = Color::White();
    canvas.DrawPaint(paint);

    paint.color = color;
    paint.style = Paint::Style::kStroke;
    paint.stroke_width = 10;

    Path path = PathBuilder{}
                    .MoveTo({20, 20})
                    .QuadraticCurveTo({60, 20}, {60, 60})
                    .Close()
                    .MoveTo({60, 20})
                    .QuadraticCurveTo({60, 60}, {20, 60})
                    .TakePath();

    canvas.Scale(Vector2(scale, scale));

    if (add_circle_clip) {
      auto [handle_a, handle_b] = IMPELLER_PLAYGROUND_LINE(
          Point(60, 300), Point(600, 300), 20, Color::Red(), Color::Red());

      auto screen_to_canvas = canvas.GetCurrentTransformation().Invert();
      Point point_a = screen_to_canvas * handle_a;
      Point point_b = screen_to_canvas * handle_b;

      Point middle = (point_a + point_b) / 2;
      auto radius = point_a.GetDistance(middle);
      canvas.ClipPath(PathBuilder{}.AddCircle(middle, radius).TakePath());
    }

    for (auto join :
         {SolidStrokeContents::Join::kBevel, SolidStrokeContents::Join::kRound,
          SolidStrokeContents::Join::kMiter}) {
      paint.stroke_join = join;
      for (auto cap :
           {SolidStrokeContents::Cap::kButt, SolidStrokeContents::Cap::kSquare,
            SolidStrokeContents::Cap::kRound}) {
        paint.stroke_cap = cap;
        canvas.DrawPath(path, paint);
        canvas.Translate({80, 0});
      }
      canvas.Translate({-240, 60});
    }

    return renderer.Render(canvas.EndRecordingAsPicture(), pass);
  };

  ASSERT_TRUE(OpenPlaygroundHere(callback));
}

TEST_P(AiksTest, CoverageOriginShouldBeAccountedForInSubpasses) {
  auto callback = [](AiksContext& renderer, RenderPass& pass) {
    Canvas canvas;
    Paint alpha;
    alpha.color = Color::Red().WithAlpha(0.5);

    auto current = Point{25, 25};
    const auto offset = Point{25, 25};
    const auto size = Size(100, 100);

    auto [b0, b1] = IMPELLER_PLAYGROUND_LINE(Point(40, 40), Point(160, 160), 10,
                                             Color::White(), Color::White());
    auto bounds = Rect::MakeLTRB(b0.x, b0.y, b1.x, b1.y);

    canvas.DrawRect(bounds, Paint{.color = Color::Yellow(),
                                  .stroke_width = 5.0f,
                                  .style = Paint::Style::kStroke});

    canvas.SaveLayer(alpha, bounds);

    canvas.DrawRect({current, size}, Paint{.color = Color::Red()});
    canvas.DrawRect({current += offset, size}, Paint{.color = Color::Green()});
    canvas.DrawRect({current += offset, size}, Paint{.color = Color::Blue()});

    canvas.Restore();

    return renderer.Render(canvas.EndRecordingAsPicture(), pass);
  };

  ASSERT_TRUE(OpenPlaygroundHere(callback));
}

TEST_P(AiksTest, DrawRectStrokesRenderCorrectly) {
  Canvas canvas;
  Paint paint;
  paint.color = Color::Red();
  paint.style = Paint::Style::kStroke;
  paint.stroke_width = 10;

  canvas.Translate({100, 100});
  canvas.DrawPath(PathBuilder{}.AddRect(Rect::MakeSize({100, 100})).TakePath(),
                  paint);

  ASSERT_TRUE(OpenPlaygroundHere(canvas.EndRecordingAsPicture()));
}

}  // namespace testing
}  // namespace impeller
