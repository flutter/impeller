// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "flutter/fml/macros.h"
#include "impeller/geometry/rect.h"

namespace impeller {

class ContentContext;
struct ContentContextOptions;
class Entity;
class Surface;
class RenderPass;

ContentContextOptions OptionsFromPass(const RenderPass& pass);

ContentContextOptions OptionsFromPassAndEntity(const RenderPass& pass,
                                               const Entity& entity);

class Contents {
 public:
  Contents();

  virtual ~Contents();

  virtual bool Render(const ContentContext& renderer,
                      const Entity& entity,
                      RenderPass& pass) const = 0;

  /// @brief Returns true if this Contents is a FilterContents. This is useful
  ///        for downcasting to FilterContents without RTTI.
  virtual bool IsFilter() const;

  /// @brief Get the bounding rectangle that this contents modifies in screen
  ///        space.
  virtual Rect GetBounds(const Entity& entity) const;

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(Contents);
};

}  // namespace impeller
