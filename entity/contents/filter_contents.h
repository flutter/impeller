// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "impeller/entity/contents/contents.h"
#include "impeller/renderer/formats.h"

namespace impeller {

class FilterContents : public Contents {
 public:
  using InputTextures = std::vector<
      std::variant<std::shared_ptr<Texture>, std::shared_ptr<FilterContents>>>;

  FilterContents();

  ~FilterContents();

  void SetTextures(InputTextures input_textures);

  // |Contents|
  bool Render(const ContentContext& renderer,
              const Entity& entity,
              RenderPass& pass) const override;

 private:
  virtual bool RenderFilter(const ContentContext& renderer,
                            const Entity& entity,
                            RenderPass& pass) const = 0;

  InputTextures input_textures_;
  std::vector<std::shared_ptr<Texture>> inpur_textures_;

  FML_DISALLOW_COPY_AND_ASSIGN(FilterContents);
};

}  // namespace impeller
