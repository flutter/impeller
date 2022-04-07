// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "impeller/entity/contents/filters/filter_input.h"
#include "impeller/entity/entity.h"
#include "impeller/renderer/formats.h"

namespace impeller {

class Pipeline;

class FilterContents : public Contents {
 public:
  enum class BlurStyle {
    /// Blurred inside and outside.
    kNormal,
    /// Solid inside, blurred outside.
    kSolid,
    /// Nothing inside, blurred outside.
    kOuter,
    /// Blurred inside, nothing outside.
    kInner,
  };

  /// 1 / sqrt(3)
  /// This is the Gaussian blur standard deviation cutoff expected by Flutter:
  /// https://api.flutter.dev/flutter/dart-ui/Shadow/convertRadiusToSigma.html
  constexpr static float kBlurSigmaScale = 0.57735026919;

  struct Radius;

  struct Sigma {
    Scalar sigma = 0.0;

    constexpr Sigma() = default;

    explicit constexpr Sigma(Scalar p_sigma) : sigma(p_sigma) {}

    constexpr operator Radius() const {
      return Radius{sigma > 0.5f ? (sigma - 0.5f) / kBlurSigmaScale : 0.0f};
    };
  };

  struct Radius {
    Scalar radius = 0.0;

    constexpr Radius() = default;

    explicit constexpr Radius(Scalar p_radius) : radius(p_radius) {}

    constexpr operator Sigma() const {
      return Sigma{radius > 0 ? kBlurSigmaScale * radius + 0.5f : 0.0f};
    };
  };

  static std::shared_ptr<FilterContents> MakeBlend(Entity::BlendMode blend_mode,
                                                   FilterInput::Vector inputs);

  static std::shared_ptr<FilterContents> MakeDirectionalGaussianBlur(
      FilterInput::Ref input,
      Sigma sigma,
      Vector2 direction,
      BlurStyle blur_style = BlurStyle::kNormal,
      FilterInput::Ref alpha_mask = nullptr);

  static std::shared_ptr<FilterContents> MakeGaussianBlur(
      FilterInput::Ref input,
      Sigma sigma_x,
      Sigma sigma_y,
      BlurStyle blur_style = BlurStyle::kNormal);

  FilterContents();

  ~FilterContents() override;

  /// @brief The input texture sources for this filter. Each input's emitted
  ///        texture is expected to have premultiplied alpha colors.
  ///
  ///        The number of required or optional textures depends on the
  ///        particular filter's implementation.
  void SetInputs(FilterInput::Vector inputs);

  // |Contents|
  bool Render(const ContentContext& renderer,
              const Entity& entity,
              RenderPass& pass) const override;

  // |Contents|
  std::optional<Rect> GetCoverage(const Entity& entity) const override;

  // |Contents|
  virtual std::optional<Snapshot> RenderToTexture(
      const ContentContext& renderer,
      const Entity& entity) const override;

 private:
  /// @brief Takes a set of zero or more input textures and writes to an output
  ///        texture.
  virtual bool RenderFilter(const FilterInput::Vector& inputs,
                            const ContentContext& renderer,
                            const Entity& entity,
                            RenderPass& pass,
                            const Rect& bounds) const = 0;

  FilterInput::Vector inputs_;
  Rect destination_;

  FML_DISALLOW_COPY_AND_ASSIGN(FilterContents);
};

}  // namespace impeller
