// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/entity/contents/filters/filter_contents.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <tuple>

#include "flutter/fml/logging.h"
#include "impeller/base/validation.h"
#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/contents/filters/blend_filter_contents.h"
#include "impeller/entity/contents/filters/gaussian_blur_filter_contents.h"
#include "impeller/entity/contents/texture_contents.h"
#include "impeller/entity/entity.h"
#include "impeller/geometry/path_builder.h"
#include "impeller/renderer/command_buffer.h"
#include "impeller/renderer/formats.h"
#include "impeller/renderer/render_pass.h"

namespace impeller {

std::shared_ptr<FilterContents> FilterContents::MakeBlend(
    Entity::BlendMode blend_mode,
    InputTextures input_textures) {
  if (blend_mode > Entity::BlendMode::kLastAdvancedBlendMode) {
    VALIDATION_LOG << "Invalid blend mode " << static_cast<int>(blend_mode)
                   << " passed to FilterContents::MakeBlend.";
    return nullptr;
  }

  if (input_textures.size() < 2 ||
      blend_mode <= Entity::BlendMode::kLastPipelineBlendMode) {
    auto blend = std::make_shared<BlendFilterContents>();
    blend->SetInputTextures(input_textures);
    blend->SetBlendMode(blend_mode);
    return blend;
  }

  if (blend_mode <= Entity::BlendMode::kLastAdvancedBlendMode) {
    InputVariant blend = input_textures[0];
    for (auto in_i = input_textures.begin() + 1; in_i < input_textures.end();
         in_i++) {
      auto new_blend = std::make_shared<BlendFilterContents>();
      new_blend->SetInputTextures({blend, *in_i});
      new_blend->SetBlendMode(blend_mode);
      blend = new_blend;
    }
    auto contents = std::get<std::shared_ptr<Contents>>(blend);
    // This downcast is safe because we know blend is a BlendFilterContents.
    return std::static_pointer_cast<FilterContents>(contents);
  }

  FML_UNREACHABLE();
}

std::shared_ptr<FilterContents> FilterContents::MakeDirectionalGaussianBlur(
    InputVariant input_texture,
    Vector2 blur_vector) {
  auto blur = std::make_shared<DirectionalGaussianBlurFilterContents>();
  blur->SetInputTextures({input_texture});
  blur->SetBlurVector(blur_vector);
  return blur;
}

std::shared_ptr<FilterContents> FilterContents::MakeGaussianBlur(
    InputVariant input_texture,
    Scalar sigma_x,
    Scalar sigma_y) {
  auto x_blur = MakeDirectionalGaussianBlur(input_texture, Point(sigma_x, 0));
  return MakeDirectionalGaussianBlur(x_blur, Point(0, sigma_y));
}

FilterContents::FilterContents() = default;

FilterContents::~FilterContents() = default;

void FilterContents::SetInputTextures(InputTextures input_textures) {
  input_textures_ = std::move(input_textures);
}

bool FilterContents::IsFilter() const {
  return true;
};

bool FilterContents::Render(const ContentContext& renderer,
                            const Entity& entity,
                            RenderPass& pass) const {
  // Run the filter.

  auto maybe_texture = RenderFilterToTexture(renderer, entity, pass);
  if (!maybe_texture.has_value()) {
    return false;
  }
  auto& texture = maybe_texture.value();

  // Draw the result texture, respecting the transform and clip stack.

  auto contents = std::make_shared<TextureContents>();
  contents->SetTexture(texture);
  contents->SetSourceRect(IRect::MakeSize(texture->GetSize()));

  Entity e;
  e.SetPath(PathBuilder{}.AddRect(GetBounds(entity)).GetCurrentPath());
  e.SetBlendMode(entity.GetBlendMode());
  e.SetStencilDepth(entity.GetStencilDepth());
  return contents->Render(renderer, e, pass);
}

Rect FilterContents::GetBoundsForInput(const Entity& entity,
                                       const InputVariant& input) {
  if (auto contents = std::get_if<std::shared_ptr<Contents>>(&input)) {
    return contents->get()->GetBounds(entity);
  }

  if (auto texture = std::get_if<std::shared_ptr<Texture>>(&input)) {
    auto points = entity.GetPath().GetBoundingBox()->GetPoints();

    const auto& transform = entity.GetTransformation();
    for (uint i = 0; i < points.size(); i++) {
      points[i] = transform * points[i];
    }
    return Rect::MakePointBounds({points.begin(), points.end()});
  }

  FML_UNREACHABLE();
}

Rect FilterContents::GetBounds(const Entity& entity) const {
  // The default bounds of FilterContents is just the union of its inputs.
  // FilterContents implementations may choose to increase the bounds in any
  // direction, but it should never

  if (input_textures_.empty()) {
    return Rect();
  }

  Rect result = GetBoundsForInput(entity, input_textures_.front());
  for (auto input_i = input_textures_.begin() + 1;
       input_i < input_textures_.end(); input_i++) {
    result.Union(GetBoundsForInput(entity, *input_i));
  }

  return result;
}

using SubpassCallback = std::function<bool(const ContentContext&, RenderPass&)>;

static std::optional<std::shared_ptr<Texture>> MakeSubpass(
    const ContentContext& renderer,
    ISize texture_size,
    SubpassCallback subpass_callback) {
  auto context = renderer.GetContext();

  auto subpass_target = RenderTarget::CreateOffscreen(*context, texture_size);
  auto subpass_texture = subpass_target.GetRenderTargetTexture();
  if (!subpass_texture) {
    return std::nullopt;
  }

  auto sub_command_buffer = context->CreateRenderCommandBuffer();
  sub_command_buffer->SetLabel("Offscreen Filter Command Buffer");
  if (!sub_command_buffer) {
    return std::nullopt;
  }

  auto sub_renderpass = sub_command_buffer->CreateRenderPass(subpass_target);
  if (!sub_renderpass) {
    return std::nullopt;
  }
  sub_renderpass->SetLabel("OffscreenFilterPass");

  if (!subpass_callback(renderer, *sub_renderpass)) {
    return std::nullopt;
  }

  if (!sub_renderpass->EncodeCommands(*context->GetTransientsAllocator())) {
    return std::nullopt;
  }

  if (!sub_command_buffer->SubmitCommands()) {
    return std::nullopt;
  }

  return subpass_texture;
}

static std::optional<std::tuple<std::shared_ptr<Texture>, Rect>>
ResolveTextureForInput(const ContentContext& renderer,
                       const Entity& entity,
                       RenderPass& pass,
                       const Rect& pass_bounds,
                       FilterContents::InputVariant input) {
  auto input_bounds = FilterContents::GetBoundsForInput(entity, input);
  auto input_relative =
      Rect(input_bounds.origin - pass_bounds.origin, input_bounds.size);

  if (auto contents = std::get_if<std::shared_ptr<Contents>>(&input)) {
    // If the input is a filter, recurse.
    if (contents->get()->IsFilter()) {
      auto filter = std::static_pointer_cast<FilterContents>(*contents);
      auto texture = filter->RenderFilterToTexture(renderer, entity, pass);
      if (!texture.has_value()) {
        return std::nullopt;
      }
      return std::make_tuple(texture.value(), input_relative);
    }

    // If the input is non-filter contents, render it into a texture.
    auto texture = MakeSubpass(
        renderer, ISize(input_relative.size),
        [contents = *contents, entity, input_relative](
            const ContentContext& renderer, RenderPass& pass) -> bool {
          Entity sub_entity;
          sub_entity.SetPath(entity.GetPath());
          sub_entity.SetBlendMode(Entity::BlendMode::kSource);
          sub_entity.SetTransformation(
              Matrix::MakeTranslation(Vector3(-input_relative.origin)) *
              entity.GetTransformation());
          return contents->Render(renderer, sub_entity, pass);
        });
    if (!texture.has_value()) {
      return std::nullopt;
    }
    return std::make_tuple(texture.value(), input_relative);
  }

  if (auto input_texture = std::get_if<std::shared_ptr<Texture>>(&input)) {
    // If the input is a texture, render the version of it which is transformed.
    auto texture = MakeSubpass(
        renderer, ISize(input_relative.size),
        [texture = *input_texture, entity, input_bounds](
            const ContentContext& renderer, RenderPass& pass) -> bool {
          TextureContents contents;
          contents.SetTexture(texture);
          contents.SetSourceRect(IRect::MakeSize(texture->GetSize()));
          Entity sub_entity;
          sub_entity.SetPath(entity.GetPath());
          sub_entity.SetBlendMode(Entity::BlendMode::kSource);
          sub_entity.SetTransformation(
              Matrix::MakeTranslation(Vector3(-input_bounds.origin)) *
              entity.GetTransformation());
          return contents.Render(renderer, sub_entity, pass);
        });
    if (!texture.has_value()) {
      return std::nullopt;
    }
    return std::make_tuple(texture.value(), input_relative);
  }

  FML_UNREACHABLE();
}

std::optional<std::shared_ptr<Texture>> FilterContents::RenderFilterToTexture(
    const ContentContext& renderer,
    const Entity& entity,
    RenderPass& pass) const {
  auto bounds = GetBounds(entity);
  if (bounds.IsZero()) {
    return std::nullopt;
  }

  // Resolve all inputs as textures.

  std::vector<std::tuple<std::shared_ptr<Texture>, Rect>> input_textures;

  input_textures.reserve(input_textures_.size());
  for (const auto& input : input_textures_) {
    auto texture_and_offset =
        ResolveTextureForInput(renderer, entity, pass, bounds, input);
    if (!texture_and_offset.has_value()) {
      continue;
    }

    input_textures.push_back(texture_and_offset.value());
  }

  // Create a new texture and render the filter to it.

  return MakeSubpass(
      renderer, ISize(GetBounds(entity).size),
      [=](const ContentContext& renderer, RenderPass& pass) -> bool {
        return RenderFilter(input_textures, renderer, pass,
                            entity.GetTransformation());
      });
}

}  // namespace impeller
