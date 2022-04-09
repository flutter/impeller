// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/typographer/text_frame.h"

namespace impeller {

TextFrame::TextFrame() = default;

TextFrame::~TextFrame() = default;

const Rect& TextFrame::GetBounds() const {
  return bounds_;
}

void TextFrame::SetBounds(Rect bounds) {
  bounds_ = std::move(bounds);
}

bool TextFrame::AddTextRun(TextRun run) {
  if (!run.IsValid()) {
    return false;
  }
  runs_.emplace_back(std::move(run));
  return true;
}

size_t TextFrame::GetRunCount() const {
  return runs_.size();
}

const std::vector<TextRun>& TextFrame::GetRuns() const {
  return runs_;
}

}  // namespace impeller
