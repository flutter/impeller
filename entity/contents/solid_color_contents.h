// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "flutter/fml/macros.h"
#include "geometry/path.h"
#include "impeller/entity/contents/contents.h"
#include "impeller/entity/solid_stroke.vert.h"
#include "impeller/geometry/color.h"
#include "impeller/geometry/path_component.h"
#include "impeller/geometry/point.h"
#include "impeller/geometry/rect.h"
#include "impeller/renderer/texture.h"
#include "impeller/typographer/glyph_atlas.h"
#include "impeller/typographer/text_frame.h"

namespace impeller {

class SolidColorContents final : public Contents {
 public:
  SolidColorContents();

  ~SolidColorContents() override;

  static std::unique_ptr<SolidColorContents> Make(Color color);

  static VertexBuffer CreateSolidFillVertices(const Path& path,
                                              HostBuffer& buffer);

  void SetColor(Color color);

  const Color& GetColor() const;

  // |Contents|
  bool Render(const ContentContext& renderer,
              const Entity& entity,
              RenderPass& pass) const override;

 private:
  Color color_;

  FML_DISALLOW_COPY_AND_ASSIGN(SolidColorContents);
};

}  // namespace impeller
