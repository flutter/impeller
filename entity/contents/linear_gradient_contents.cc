// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "linear_gradient_contents.h"

#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/entity.h"
#include "impeller/renderer/render_pass.h"
#include "impeller/renderer/tessellator.h"

namespace impeller {

LinearGradientContents::LinearGradientContents() = default;

LinearGradientContents::~LinearGradientContents() = default;

void LinearGradientContents::SetEndPoints(Point start_point, Point end_point) {
  start_point_ = start_point;
  end_point_ = end_point;
}

void LinearGradientContents::SetColors(std::vector<Color> colors) {
  colors_ = std::move(colors);
  if (colors_.empty()) {
    colors_.push_back(Color::Black());
    colors_.push_back(Color::Black());
  } else if (colors_.size() < 2u) {
    colors_.push_back(colors_.back());
  }
}

const std::vector<Color>& LinearGradientContents::GetColors() const {
  return colors_;
}

bool LinearGradientContents::Render(const ContentContext& renderer,
                                    const Entity& entity,
                                    RenderPass& pass) const {
  using VS = GradientFillPipeline::VertexShader;
  using FS = GradientFillPipeline::FragmentShader;

  auto vertices_builder = VertexBufferBuilder<VS::PerVertexData>();
  {
    auto result = Tessellator{entity.GetPath().GetFillType()}.Tessellate(
        entity.GetPath().CreatePolyline(), [&vertices_builder](Point point) {
          VS::PerVertexData vtx;
          vtx.vertices = point;
          vertices_builder.AppendVertex(vtx);
        });
    if (!result) {
      return false;
    }
  }

  VS::FrameInfo frame_info;
  frame_info.mvp = Matrix::MakeOrthographic(pass.GetRenderTargetSize()) *
                   entity.GetTransformation();

  FS::GradientInfo gradient_info;
  gradient_info.start_point = start_point_;
  gradient_info.end_point = end_point_;
  gradient_info.start_color = colors_[0];
  gradient_info.end_color = colors_[1];

  Command cmd;
  cmd.label = "LinearGradientFill";
  cmd.pipeline = renderer.GetGradientFillPipeline(OptionsFromPass(pass));
  cmd.stencil_reference = entity.GetStencilDepth();
  cmd.BindVertices(
      vertices_builder.CreateVertexBuffer(pass.GetTransientsBuffer()));
  cmd.primitive_type = PrimitiveType::kTriangle;
  FS::BindGradientInfo(
      cmd, pass.GetTransientsBuffer().EmplaceUniform(gradient_info));
  VS::BindFrameInfo(cmd, pass.GetTransientsBuffer().EmplaceUniform(frame_info));
  return pass.AddCommand(std::move(cmd));
}

}  // namespace impeller
