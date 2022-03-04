// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "impeller/entity/contents/contents.h"
#include "impeller/geometry/color.h"
#include "impeller/geometry/matrix.h"
#include "impeller/geometry/path.h"
#include "impeller/geometry/rect.h"
#include "impeller/image/decompressed_image.h"

namespace impeller {

class Renderer;
class RenderPass;

class Entity {
 public:
  Entity();

  ~Entity();

  const Matrix& GetTransformation() const;

  void SetTransformation(const Matrix& transformation);

  const Path& GetPath() const;

  void SetPath(Path path);

  void SetAddsToCoverage(bool adds);

  bool AddsToCoverage() const;

  std::optional<Rect> GetCoverage() const;

  void SetContents(std::shared_ptr<Contents> contents);

  const std::shared_ptr<Contents>& GetContents() const;

  void SetStencilDepth(uint32_t stencil_depth);

  void IncrementStencilDepth(uint32_t increment);

  uint32_t GetStencilDepth() const;

  bool Render(ContentContext& renderer, RenderPass& parent_pass) const;

 private:
  Matrix transformation_;
  std::shared_ptr<Contents> contents_;
  Path path_;
  uint32_t stencil_depth_ = 0u;
  bool adds_to_coverage_ = true;
};

}  // namespace impeller
