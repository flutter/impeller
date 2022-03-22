// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/display_list/display_list_playground.h"

#include "flutter/testing/testing.h"
#include "impeller/aiks/aiks_context.h"
#include "impeller/display_list/display_list_dispatcher.h"
#include "third_party/skia/include/core/SkData.h"

namespace impeller {

DisplayListPlayground::DisplayListPlayground() = default;

DisplayListPlayground::~DisplayListPlayground() = default;

bool DisplayListPlayground::OpenPlaygroundHere(
    flutter::DisplayListBuilder& builder) {
  return OpenPlaygroundHere(builder.Build());
}

bool DisplayListPlayground::OpenPlaygroundHere(
    sk_sp<flutter::DisplayList> list) {
  if (!Playground::is_enabled()) {
    return true;
  }

  if (!list) {
    return false;
  }

  DisplayListDispatcher dispatcher;
  list->Dispatch(dispatcher);
  auto picture = dispatcher.EndRecordingAsPicture();

  AiksContext context(GetContext());
  if (!context.IsValid()) {
    return false;
  }
  return Playground::OpenPlaygroundHere(
      [&picture, &context](RenderPass& pass) -> bool {
        return context.Render(picture, pass);
      });
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

SkFont DisplayListPlayground::CreateTestFontOfSize(SkScalar scalar) {
  static constexpr const char* kTestFontFixture = "Roboto-Regular.ttf";
  auto mapping = OpenFixtureAsSkData(kTestFontFixture);
  FML_CHECK(mapping);
  return SkFont{SkTypeface::MakeFromData(mapping), scalar};
}

SkFont DisplayListPlayground::CreateTestFont() {
  return CreateTestFontOfSize(50);
}

}  // namespace impeller
