// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "filter_contents.h"

#include <memory>
#include <optional>
#include <variant>

#include "base/validation.h"
#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/contents/solid_color_contents.h"
#include "impeller/entity/entity.h"
#include "impeller/renderer/command_buffer.h"
#include "impeller/renderer/render_pass.h"

namespace impeller {

FilterContents::FilterContents() = default;

FilterContents::~FilterContents() = default;

void FilterContents::SetTextures(InputTextures input_textures) {
  input_textures_ = std::move(input_textures);
}

static std::optional<std::shared_ptr<Texture>> RenderDependency(
    const ContentContext& renderer,
    const Entity& entity,
    RenderPass& pass,
    std::shared_ptr<FilterContents>& input) {
  auto context = renderer.GetContext();

  // TODO(bdero): Use texture params for size and add optional xform input for
  //              transform?
  auto subpass_target =
      RenderTarget::CreateOffscreen(*context, pass.GetRenderTargetSize());
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

  if (!input->Render(renderer, entity, *sub_renderpass)) {
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

bool FilterContents::Render(const ContentContext& renderer,
                            const Entity& entity,
                            RenderPass& pass) const {
  std::vector<std::shared_ptr<Texture>> texture_params;
  texture_params.reserve(input_textures_.size());

  // Render input dependencies.
  for (auto input : input_textures_) {
    if (std::holds_alternative<std::shared_ptr<FilterContents>>(input)) {
      auto& filter = std::get<std::shared_ptr<FilterContents>>(input);
      auto texture = RenderDependency(renderer, entity, pass, filter);
      if (!texture.has_value()) {
        return false;
      }
      texture_params.push_back(std::move(texture.value()));
    } else {
      auto& texture = std::get<std::shared_ptr<Texture>>(input);
      texture_params.push_back(std::move(texture));
    }
  }

  Command cmd;
  pass.AddCommand(std::move(cmd));
  return true;
}

}  // namespace impeller
