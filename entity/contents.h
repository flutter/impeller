// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <vector>

#include "flutter/fml/macros.h"
#include "impeller/geometry/color.h"
#include "impeller/geometry/point.h"
#include "impeller/geometry/rect.h"
#include "impeller/renderer/texture.h"
#include "impeller/typographer/glyph_atlas.h"
#include "impeller/typographer/text_frame.h"

namespace impeller {

class ContentContext;
class Entity;
class Surface;
class RenderPass;

class Contents {
 public:
  Contents();

  virtual ~Contents();

  virtual bool Render(const ContentContext& renderer,
                      const Entity& entity,
                      RenderPass& pass) const = 0;

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(Contents);
};

class LinearGradientContents final : public Contents {
 public:
  LinearGradientContents();

  ~LinearGradientContents() override;

  // |Contents|
  bool Render(const ContentContext& renderer,
              const Entity& entity,
              RenderPass& pass) const override;

  void SetEndPoints(Point start_point, Point end_point);

  void SetColors(std::vector<Color> colors);

  const std::vector<Color>& GetColors() const;

 private:
  Point start_point_;
  Point end_point_;
  std::vector<Color> colors_;

  FML_DISALLOW_COPY_AND_ASSIGN(LinearGradientContents);
};

class SolidColorContents final : public Contents {
 public:
  SolidColorContents();

  ~SolidColorContents() override;

  static std::unique_ptr<SolidColorContents> Make(Color color);

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

class TextureContents final : public Contents {
 public:
  TextureContents();

  ~TextureContents() override;

  void SetTexture(std::shared_ptr<Texture> texture);

  std::shared_ptr<Texture> GetTexture() const;

  void SetSourceRect(const IRect& source_rect);

  void SetOpacity(Scalar opacity);

  const IRect& GetSourceRect() const;

  // |Contents|
  bool Render(const ContentContext& renderer,
              const Entity& entity,
              RenderPass& pass) const override;

 public:
  std::shared_ptr<Texture> texture_;
  IRect source_rect_;
  Scalar opacity_ = 1.0f;

  FML_DISALLOW_COPY_AND_ASSIGN(TextureContents);
};

class SolidStrokeContents final : public Contents {
 public:
  SolidStrokeContents();

  ~SolidStrokeContents() override;

  void SetColor(Color color);

  const Color& GetColor() const;

  void SetStrokeSize(Scalar size);

  Scalar GetStrokeSize() const;

  // |Contents|
  bool Render(const ContentContext& renderer,
              const Entity& entity,
              RenderPass& pass) const override;

 private:
  Color color_;
  Scalar stroke_size_ = 0.0;

  FML_DISALLOW_COPY_AND_ASSIGN(SolidStrokeContents);
};

class ClipContents final : public Contents {
 public:
  ClipContents();

  ~ClipContents();

  // |Contents|
  bool Render(const ContentContext& renderer,
              const Entity& entity,
              RenderPass& pass) const override;

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(ClipContents);
};

class ClipRestoreContents final : public Contents {
 public:
  ClipRestoreContents();

  ~ClipRestoreContents();

  void SetGlyphAtlas(std::shared_ptr<GlyphAtlas> atlas);

  // |Contents|
  bool Render(const ContentContext& renderer,
              const Entity& entity,
              RenderPass& pass) const override;

 private:
  FML_DISALLOW_COPY_AND_ASSIGN(ClipRestoreContents);
};

class TextContents final : public Contents {
 public:
  TextContents();

  ~TextContents();

  void SetTextFrame(TextFrame frame);

  void SetGlyphAtlas(std::shared_ptr<GlyphAtlas> atlas);

  // |Contents|
  bool Render(const ContentContext& renderer,
              const Entity& entity,
              RenderPass& pass) const override;

 private:
  TextFrame frame_;
  std::shared_ptr<GlyphAtlas> atlas_;

  FML_DISALLOW_COPY_AND_ASSIGN(TextContents);
};

}  // namespace impeller
