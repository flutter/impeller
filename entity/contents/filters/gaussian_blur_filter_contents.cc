// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/entity/contents/filters/gaussian_blur_filter_contents.h"
#include <cmath>
#include <valarray>

#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/contents/filters/filter_contents.h"
#include "impeller/geometry/rect.h"
#include "impeller/renderer/render_pass.h"
#include "impeller/renderer/sampler_library.h"

namespace impeller {

DirectionalGaussianBlurFilterContents::DirectionalGaussianBlurFilterContents() =
    default;

DirectionalGaussianBlurFilterContents::
    ~DirectionalGaussianBlurFilterContents() = default;

void DirectionalGaussianBlurFilterContents::SetBlurVector(Vector2 blur_vector) {
  if (blur_vector.GetLengthSquared() < 1e-3f) {
    blur_vector_ = Vector2(0, 1e-3f);
    return;
  }
  blur_vector_ = blur_vector;
}

bool DirectionalGaussianBlurFilterContents::RenderFilter(
    const std::vector<std::tuple<std::shared_ptr<Texture>, Point>>&
        input_textures,
    const ContentContext& renderer,
    RenderPass& pass) const {
  using VS = GaussianBlurPipeline::VertexShader;
  using FS = GaussianBlurPipeline::FragmentShader;

  auto& host_buffer = pass.GetTransientsBuffer();

  auto size = pass.GetRenderTargetSize();
  VertexBufferBuilder<VS::PerVertexData> vtx_builder;
  vtx_builder.AddVertices({
      {Point(0, 0), Point(0, 0)},
      {Point(size.width, 0), Point(1, 0)},
      {Point(size.width, size.height), Point(1, 1)},
      {Point(0, 0), Point(0, 0)},
      {Point(size.width, size.height), Point(1, 1)},
      {Point(0, size.height), Point(0, 1)},
  });
  auto vtx_buffer = vtx_builder.CreateVertexBuffer(host_buffer);

  VS::FrameInfo frame_info;
  frame_info.texture_size = Point(size);
  frame_info.blur_radius = blur_vector_.GetLength();
  frame_info.blur_direction = blur_vector_.Normalize();

  auto sampler = renderer.GetContext()->GetSamplerLibrary()->GetSampler({});

  Command cmd;
  cmd.label = "Gaussian Blur Filter";
  auto options = OptionsFromPass(pass);
  options.blend_mode = Entity::BlendMode::kSource;
  cmd.pipeline = renderer.GetGaussianBlurPipeline(options);
  cmd.BindVertices(vtx_buffer);
  for (const auto& [texture, offset] : input_textures) {
    FS::BindTextureSampler(cmd, texture, sampler);

    frame_info.mvp =
        Matrix::MakeTranslation(offset) * Matrix::MakeOrthographic(size);
    auto uniform_view = host_buffer.EmplaceUniform(frame_info);
    VS::BindFrameInfo(cmd, uniform_view);

    pass.AddCommand(cmd);
  }

  return true;
}

Rect DirectionalGaussianBlurFilterContents::GetBounds(
    const Entity& entity) const {
  auto bounds = FilterContents::GetBounds(entity);
  auto extent = bounds.size + blur_vector_ * 2;
  return Rect(bounds.origin - blur_vector_, Size(extent.x, extent.y));
}

}  // namespace impeller
