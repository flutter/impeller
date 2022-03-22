// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "impeller/entity/contents/filters/filter_contents.h"

namespace impeller {

class DirectionalGaussianBlurFilterContents final : public FilterContents {
 public:
  DirectionalGaussianBlurFilterContents();

  ~DirectionalGaussianBlurFilterContents() override;

  void SetBlurVector(Vector2 blur_vector);

  // |Contents|
  Rect GetBounds(const Entity& entity) const override;

 private:
  // |FilterContents|
  bool RenderFilter(
      const std::vector<std::tuple<std::shared_ptr<Texture>, Point>>&
          input_textures,
      const ContentContext& renderer,
      RenderPass& pass) const override;

  Vector2 blur_vector_;

  FML_DISALLOW_COPY_AND_ASSIGN(DirectionalGaussianBlurFilterContents);
};

}  // namespace impeller
