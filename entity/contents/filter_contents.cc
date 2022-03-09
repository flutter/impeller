// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "filter_contents.h"

#include <memory>
#include <optional>
#include <variant>

#include "impeller/base/validation.h"
#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/contents/solid_color_contents.h"
#include "impeller/entity/contents/texture_contents.h"
#include "impeller/entity/entity.h"
#include "impeller/geometry/path_builder.h"
#include "impeller/renderer/command_buffer.h"
#include "impeller/renderer/render_pass.h"

namespace impeller {

FilterContents::FilterContents() = default;

FilterContents::~FilterContents() = default;

void FilterContents::SetInputTextures(InputTextures& input_textures) {
  input_textures_ = std::move(input_textures);
}

void FilterContents::SetDestination(const Rect& destination) {
  destination_ = destination;
}

bool FilterContents::Render(const ContentContext& renderer,
                            const Entity& entity,
                            RenderPass& pass) const {
  // Run the filter.

  auto maybe_texture = RenderFilterToTexture(renderer, entity, pass);
  if (!maybe_texture.has_value()) {
    return false;
  }
  auto& texture = maybe_texture.value();

  // Draw the resulting texture to the given destination rect, respecting the
  // transform and clip stack.

  auto contents = std::make_shared<TextureContents>();
  contents->SetTexture(texture);
  contents->SetSourceRect(IRect::MakeSize(texture->GetSize()));
  Entity e;
  e.SetPath(PathBuilder{}.AddRect(destination_).TakePath());
  e.SetContents(std::move(contents));
  e.SetStencilDepth(entity.GetStencilDepth());
  e.SetTransformation(entity.GetTransformation());

  return e.Render(renderer, pass);
}

std::optional<std::shared_ptr<Texture>> FilterContents::RenderFilterToTexture(
    const ContentContext& renderer,
    const Entity& entity,
    RenderPass& pass) const {
  // Resolve filter dependencies.

  std::vector<std::shared_ptr<Texture>> input_textures;
  input_textures.reserve(input_textures_.size());
  for (const auto& input : input_textures_) {
    if (std::holds_alternative<std::shared_ptr<FilterContents>>(input)) {
      auto& filter = std::get<std::shared_ptr<FilterContents>>(input);
      auto texture = filter->RenderFilterToTexture(renderer, entity, pass);
      if (!texture.has_value()) {
        return std::nullopt;
      }
      input_textures.push_back(std::move(texture.value()));
    } else {
      auto& texture = std::get<std::shared_ptr<Texture>>(input);
      input_textures.push_back(std::move(texture));
    }
  }

  // Create a new texture

  auto context = renderer.GetContext();

  auto subpass_target =
      RenderTarget::CreateOffscreen(*context, GetOutputSize());
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

  if (!RenderFilter(input_textures, renderer, *sub_renderpass)) {
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

ISize FilterContents::GetOutputSize() const {
  if (!input_textures_.empty()) {
    if (std::holds_alternative<std::shared_ptr<FilterContents>>(
            input_textures_[0])) {
      auto& filter =
          std::get<std::shared_ptr<FilterContents>>(input_textures_[0]);
      return filter->GetOutputSize();
    } else {
      auto& texture = std::get<std::shared_ptr<Texture>>(input_textures_[0]);
      return texture->GetSize();
    }
  }
  return ISize::Ceil(destination_.size);
}

}  // namespace impeller
