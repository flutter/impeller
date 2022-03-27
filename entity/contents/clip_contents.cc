// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/geometry/path_builder.h"
#include "linear_gradient_contents.h"

#include "impeller/entity/contents/clip_contents.h"
#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/contents/solid_color_contents.h"
#include "impeller/entity/entity.h"
#include "impeller/renderer/render_pass.h"

namespace impeller {

/*******************************************************************************
 ******* ClipContents
 ******************************************************************************/

ClipContents::ClipContents() = default;

ClipContents::~ClipContents() = default;

void ClipContents::SetClipOperation(Entity::ClipOperation clip_op) {
  clip_op_ = clip_op;
}

bool ClipContents::Render(const ContentContext& renderer,
                          const Entity& entity,
                          RenderPass& pass) const {
  using VS = ClipPipeline::VertexShader;

  Path clip_path = entity.GetPath();

  // For kDifference, prepend a rectangle to the path which covers the entire
  // screen in order to invert the path tessellation.
  if (clip_op_ == Entity::ClipOperation::kDifference) {
    PathBuilder path_builder;
    auto screen_points = Rect(Size(pass.GetRenderTargetSize())).GetPoints();

    // Reverse the transform that will be applied to the resulting geometry in
    // the vertex shader so that it ends up mapping to the corners of the
    // screen.
    auto inverse_transform = entity.GetTransformation().Invert();
    for (uint i = 0; i < screen_points.size(); i++) {
      screen_points[i] = inverse_transform * screen_points[i];
    }

    path_builder.AddLine(screen_points[0], screen_points[1]);
    path_builder.LineTo(screen_points[3]);
    path_builder.LineTo(screen_points[2]);
    path_builder.Close();

    path_builder.AddPath(clip_path);
    clip_path = path_builder.TakePath();
  }

  Command cmd;
  cmd.label = "Clip";
  cmd.pipeline =
      renderer.GetClipPipeline(OptionsFromPassAndEntity(pass, entity));
  cmd.stencil_reference = entity.GetStencilDepth();
  cmd.BindVertices(SolidColorContents::CreateSolidFillVertices(
      clip_path, pass.GetTransientsBuffer()));

  VS::FrameInfo info;
  // The color really doesn't matter.
  info.color = Color::SkyBlue();
  info.mvp = Matrix::MakeOrthographic(pass.GetRenderTargetSize()) *
             entity.GetTransformation();

  VS::BindFrameInfo(cmd, pass.GetTransientsBuffer().EmplaceUniform(info));

  pass.AddCommand(std::move(cmd));
  return true;
}

/*******************************************************************************
 ******* ClipRestoreContents
 ******************************************************************************/

ClipRestoreContents::ClipRestoreContents() = default;

ClipRestoreContents::~ClipRestoreContents() = default;

bool ClipRestoreContents::Render(const ContentContext& renderer,
                                 const Entity& entity,
                                 RenderPass& pass) const {
  using VS = ClipPipeline::VertexShader;

  Command cmd;
  cmd.label = "Clip Restore";
  cmd.pipeline =
      renderer.GetClipRestorePipeline(OptionsFromPassAndEntity(pass, entity));
  cmd.stencil_reference = entity.GetStencilDepth();

  // Create a rect that covers the whole render target.
  auto size = pass.GetRenderTargetSize();
  VertexBufferBuilder<VS::PerVertexData> vtx_builder;
  vtx_builder.AddVertices({
      {Point(0.0, 0.0)},
      {Point(size.width, 0.0)},
      {Point(size.width, size.height)},
      {Point(0.0, 0.0)},
      {Point(size.width, size.height)},
      {Point(0.0, size.height)},
  });
  cmd.BindVertices(vtx_builder.CreateVertexBuffer(pass.GetTransientsBuffer()));

  VS::FrameInfo info;
  // The color really doesn't matter.
  info.color = Color::SkyBlue();
  info.mvp = Matrix::MakeOrthographic(pass.GetRenderTargetSize());

  VS::BindFrameInfo(cmd, pass.GetTransientsBuffer().EmplaceUniform(info));

  pass.AddCommand(std::move(cmd));
  return true;
}

};  // namespace impeller
