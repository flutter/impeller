// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/typographer/text_render_context.h"

#include "impeller/typographer/backends/skia/text_render_context_skia.h"

namespace impeller {

std::unique_ptr<TextRenderContext> TextRenderContext::Create(
    std::shared_ptr<Context> context) {
  // There is only one backend today.
  return std::make_unique<TextRenderContextSkia>(std::move(context));
}

TextRenderContext::TextRenderContext(std::shared_ptr<Context> context)
    : context_(std::move(context)) {
  if (!context_ || !context_->IsValid()) {
    return;
  }
  is_valid_ = true;
}

TextRenderContext::~TextRenderContext() = default;

bool TextRenderContext::IsValid() const {
  return is_valid_;
}

const std::shared_ptr<Context>& TextRenderContext::GetContext() const {
  return context_;
}

std::shared_ptr<GlyphAtlas> TextRenderContext::CreateGlyphAtlas(
    const TextFrame& frame) const {
  size_t count = 0;
  FrameIterator iterator = [&]() -> const TextFrame* {
    count++;
    if (count == 1) {
      return &frame;
    }
    return nullptr;
  };
  return CreateGlyphAtlas(iterator);
}

}  // namespace impeller
