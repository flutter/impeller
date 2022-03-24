// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/entity/contents/text_contents.h"

#include "fml/logging.h"
#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/entity.h"
#include "impeller/geometry/path_builder.h"
#include "impeller/renderer/render_pass.h"
#include "impeller/renderer/sampler_library.h"
#include "impeller/tessellator/tessellator.h"
#include "impeller/typographer/glyph_atlas.h"
#include "impeller/typographer/lazy_glyph_atlas.h"

namespace impeller {

TextContents::TextContents() = default;

TextContents::~TextContents() = default;

void TextContents::SetTextFrame(TextFrame frame) {
  frame_ = std::move(frame);
}

void TextContents::SetGlyphAtlas(std::shared_ptr<GlyphAtlas> atlas) {
  atlas_ = std::move(atlas);
}

void TextContents::SetGlyphAtlas(std::shared_ptr<LazyGlyphAtlas> atlas) {
  atlas_ = std::move(atlas);
}

std::shared_ptr<GlyphAtlas> TextContents::ResolveAtlas(
    std::shared_ptr<Context> context,
    Scalar font_scale) const {
  if (auto lazy_atlas = std::get_if<std::shared_ptr<LazyGlyphAtlas>>(&atlas_)) {
    auto atlas = lazy_atlas->get()->CreateOrGetGlyphAtlas(context, font_scale);
    return atlas;
  }

  if (auto atlas = std::get_if<std::shared_ptr<GlyphAtlas>>(&atlas_)) {
    return *atlas;
  }

  return nullptr;
}

void TextContents::SetColor(Color color) {
  color_ = color;
}

bool TextContents::Render(const ContentContext& renderer,
                          const Entity& entity,
                          RenderPass& pass) const {
  if (color_.IsTransparent()) {
    return true;
  }

  //Scalar font_scale = entity.GetTransformation().GetMaxBasisLength();

  auto atlas = ResolveAtlas(renderer.GetContext(), 1);

  if (!atlas || !atlas->IsValid()) {
    VALIDATION_LOG << "Cannot render glyphs without prepared atlas.";
    return false;
  }

  using VS = GlyphAtlasPipeline::VertexShader;
  using FS = GlyphAtlasPipeline::FragmentShader;

  // Information shared by all glyph draw calls.
  Command cmd;
  cmd.label = "TextFrame";
  cmd.primitive_type = PrimitiveType::kTriangle;
  cmd.pipeline =
      renderer.GetGlyphAtlasPipeline(OptionsFromPassAndEntity(pass, entity));
  cmd.stencil_reference = entity.GetStencilDepth();

  // Common vertex uniforms for all glyphs.
  VS::FrameInfo frame_info;
  frame_info.mvp = Matrix::MakeOrthographic(pass.GetRenderTargetSize()) *
                   entity.GetTransformation();
  frame_info.atlas_size =
      Point{static_cast<Scalar>(atlas->GetTexture()->GetSize().width),
            static_cast<Scalar>(atlas->GetTexture()->GetSize().height)};
  frame_info.text_color = ToVector(color_);
  VS::BindFrameInfo(cmd, pass.GetTransientsBuffer().EmplaceUniform(frame_info));

  // Common fragment uniforms for all glyphs.
  FS::BindGlyphAtlasSampler(
      cmd,                                                        // command
      atlas->GetTexture(),                                        // texture
      renderer.GetContext()->GetSamplerLibrary()->GetSampler({})  // sampler
  );

  // Common vertex information for all glyphs.
  // All glyphs are given the same vertex information in the form of a
  // unit-sized quad. The size of the glyph is specified in per instance data
  // and the vertex shader uses this to size the glyph correctly. The
  // interpolated vertex information is also used in the fragment shader to
  // sample from the glyph atlas.
  {
    VertexBufferBuilder<VS::PerVertexData> vertex_builder;
    if (!Tessellator{}.Tessellate(
            FillType::kPositive,
            PathBuilder{}
                .AddRect(Rect::MakeXYWH(0.0, 0.0, 1.0, 1.0))
                .TakePath()
                .CreatePolyline(),
            [&vertex_builder](Point point) {
              VS::PerVertexData vtx;
              vtx.unit_vertex = point;
              vertex_builder.AppendVertex(std::move(vtx));
            })) {
      return false;
    }
    auto dummy = vertex_builder.CreateVertexBuffer(pass.GetTransientsBuffer());
    auto vertex_buffer = dummy;
    if (!vertex_buffer) {
      return false;
    }
    cmd.BindVertices(std::move(vertex_buffer));
  }

  size_t instance_count = 0u;
  std::vector<Matrix> glyph_positions;
  std::vector<Point> glyph_sizes;
  std::vector<Point> atlas_positions;
  std::vector<Point> atlas_glyph_sizes;

  for (const auto& run : frame_.GetRuns()) {
    auto font = run.GetFont();
    //font.ScaleMetrics(font_scale);

    auto glyph_size = ISize::Ceil(font.GetMetrics().GetBoundingBox().size);
    for (const auto& glyph_position : run.GetGlyphPositions()) {
      FontGlyphPair font_glyph_pair{font, glyph_position.glyph};
      auto atlas_glyph_pos = atlas->FindFontGlyphPosition(font_glyph_pair);
      if (!atlas_glyph_pos.has_value()) {
        // TODO(100729): It's possible for this to happen sometimes due to our
        // method of scaling and the way hashes are calculated. In particular,
        // glyphs with animated rotation are running into this often. See the
        // bug for more details about how to reproduce this in gallery.
        FML_DLOG(ERROR) << "Glyph lookup failed."
                        << "\n    Glyph index: " << glyph_position.glyph.index
                        << "\n    scale: " << font.GetMetrics().scale
                        << "\n    point_size: " << font.GetMetrics().point_size
                        << "\n    ascent: " << font.GetMetrics().ascent
                        << "\n    descent: " << font.GetMetrics().descent
                        << "\n    min_extent: "
                        << font.GetMetrics().min_extent.x << " "
                        << font.GetMetrics().min_extent.y
                        << "\n    max_extent: "
                        << font.GetMetrics().max_extent.x << " "
                        << font.GetMetrics().max_extent.y;
        continue;
      }
      instance_count++;
      glyph_positions.emplace_back(glyph_position.position.Translate(
          {font.GetMetrics().min_extent.x, font.GetMetrics().ascent, 0.0}));
      glyph_sizes.emplace_back(Point{static_cast<Scalar>(glyph_size.width),
                                     static_cast<Scalar>(glyph_size.height)});
      atlas_positions.emplace_back(atlas_glyph_pos->origin);
      atlas_glyph_sizes.emplace_back(
          Point{atlas_glyph_pos->size.width, atlas_glyph_pos->size.height});
    }
  }

  cmd.instance_count = instance_count;
  VS::BindGlyphPositions(
      cmd, pass.GetTransientsBuffer().EmplaceStorageBuffer(glyph_positions));
  VS::BindGlyphSizes(
      cmd, pass.GetTransientsBuffer().EmplaceStorageBuffer(glyph_sizes));
  VS::BindAtlasPositions(
      cmd, pass.GetTransientsBuffer().EmplaceStorageBuffer(atlas_positions));
  VS::BindAtlasGlyphSizes(
      cmd, pass.GetTransientsBuffer().EmplaceStorageBuffer(atlas_glyph_sizes));

  if (!pass.AddCommand(cmd)) {
    return false;
  }

  return true;
}

}  // namespace impeller
