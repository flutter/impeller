// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "imgui_impl_impeller.h"

#include <algorithm>
#include <climits>
#include <memory>
#include <vector>

#include "imgui_raster.frag.h"
#include "imgui_raster.vert.h"
#include "third_party/imgui/imgui.h"

#include "impeller/geometry/matrix.h"
#include "impeller/geometry/point.h"
#include "impeller/geometry/rect.h"
#include "impeller/geometry/size.h"
#include "impeller/renderer/allocator.h"
#include "impeller/renderer/command.h"
#include "impeller/renderer/context.h"
#include "impeller/renderer/formats.h"
#include "impeller/renderer/pipeline_builder.h"
#include "impeller/renderer/pipeline_library.h"
#include "impeller/renderer/range.h"
#include "impeller/renderer/render_pass.h"
#include "impeller/renderer/sampler.h"
#include "impeller/renderer/sampler_library.h"
#include "impeller/renderer/texture.h"
#include "impeller/renderer/texture_descriptor.h"
#include "impeller/renderer/vertex_buffer.h"

struct ImGui_ImplImpeller_Data {
  std::shared_ptr<impeller::Context> context;
  std::shared_ptr<impeller::Texture> font_texture;
  std::shared_ptr<impeller::Pipeline> pipeline;
  std::shared_ptr<const impeller::Sampler> sampler;
};

static ImGui_ImplImpeller_Data* ImGui_ImplImpeller_GetBackendData() {
  return ImGui::GetCurrentContext()
             ? static_cast<ImGui_ImplImpeller_Data*>(
                   ImGui::GetIO().BackendRendererUserData)
             : nullptr;
}

bool ImGui_ImplImpeller_Init(std::shared_ptr<impeller::Context> context) {
  ImGuiIO& io = ImGui::GetIO();
  IM_ASSERT(io.BackendRendererUserData == nullptr &&
            "Already initialized a renderer backend!");

  // Setup backend capabilities flags
  auto* bd = new ImGui_ImplImpeller_Data();
  io.BackendRendererUserData = reinterpret_cast<void*>(bd);
  io.BackendRendererName = "imgui_impl_impeller";
  io.BackendFlags |=
      ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the
                                               // ImDrawCmd::VtxOffset field,
                                               // allowing for large meshes.

  bd->context = context;

  // Generate/upload the font atlas.
  {
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    auto texture_descriptor = impeller::TextureDescriptor{};
    texture_descriptor.format = impeller::PixelFormat::kR8G8B8A8UNormInt;
    texture_descriptor.size = {width, height};
    texture_descriptor.mip_count = 1u;

    bd->font_texture = context->GetPermanentsAllocator()->CreateTexture(
        impeller::StorageMode::kHostVisible, texture_descriptor);
    IM_ASSERT(bd->font_texture != nullptr &&
              "Could not allocate ImGui font texture.");

    [[maybe_unused]] bool uploaded = bd->font_texture->SetContents(
        pixels, texture_descriptor.GetByteSizeOfBaseMipLevel());
    IM_ASSERT(uploaded &&
              "Could not upload ImGui font texture to device memory.");
  }

  // Build the raster pipeline.
  {
    auto desc = impeller::PipelineBuilder<impeller::ImguiRasterVertexShader,
                                          impeller::ImguiRasterFragmentShader>::
        MakeDefaultPipelineDescriptor(*context);
    desc->SetSampleCount(impeller::SampleCount::kCount4);
    bd->pipeline =
        context->GetPipelineLibrary()->GetRenderPipeline(std::move(desc)).get();
    IM_ASSERT(bd->pipeline != nullptr && "Could not create ImGui pipeline.");

    bd->sampler = context->GetSamplerLibrary()->GetSampler({});
    IM_ASSERT(bd->pipeline != nullptr && "Could not create ImGui sampler.");
  }

  return true;
}

void ImGui_ImplImpeller_Shutdown() {
  auto* bd = ImGui_ImplImpeller_GetBackendData();
  IM_ASSERT(bd != nullptr &&
            "No renderer backend to shutdown, or already shutdown?");
  delete bd;
}

void ImGui_ImplImpeller_RenderDrawData(ImDrawData* draw_data,
                                       impeller::RenderPass& render_pass) {
  if (draw_data->CmdListsCount == 0) {
    return;  // Nothing to render.
  }

  using VS = impeller::ImguiRasterVertexShader;
  using FS = impeller::ImguiRasterFragmentShader;

  auto* bd = ImGui_ImplImpeller_GetBackendData();
  IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplImpeller_Init()?");

  size_t total_vtx_bytes = draw_data->TotalVtxCount * sizeof(ImDrawVert);
  size_t total_idx_bytes = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
  if (!total_vtx_bytes || !total_idx_bytes) {
    return;  // Nothing to render.
  }

  // Allocate buffer for vertices + indices.
  auto buffer = bd->context->GetTransientsAllocator()->CreateBuffer(
      impeller::StorageMode::kHostVisible, total_vtx_bytes + total_idx_bytes);
  buffer->SetLabel(impeller::SPrintF("ImGui vertex+index buffer"));

  VS::UniformBuffer uniforms;
  uniforms.mvp = impeller::Matrix::MakeOrthographic(
      impeller::Size(draw_data->DisplaySize.x, draw_data->DisplaySize.y));
  uniforms.mvp = uniforms.mvp.Translate(
      -impeller::Vector3(draw_data->DisplayPos.x, draw_data->DisplayPos.y));

  size_t vertex_buffer_offset = 0;
  size_t index_buffer_offset = total_vtx_bytes;

  for (int draw_list_i = 0; draw_list_i < draw_data->CmdListsCount;
       draw_list_i++) {
    const ImDrawList* cmd_list = draw_data->CmdLists[draw_list_i];

    auto draw_list_vtx_bytes =
        static_cast<size_t>(cmd_list->VtxBuffer.size_in_bytes());
    auto draw_list_idx_bytes =
        static_cast<size_t>(cmd_list->IdxBuffer.size_in_bytes());

    if (!buffer->CopyHostBuffer(
            reinterpret_cast<uint8_t*>(cmd_list->VtxBuffer.Data),
            impeller::Range{0, draw_list_vtx_bytes}, vertex_buffer_offset)) {
      IM_ASSERT(false && "Could not copy vertices to buffer.");
    }
    if (!buffer->CopyHostBuffer(
            reinterpret_cast<uint8_t*>(cmd_list->IdxBuffer.Data),
            impeller::Range{0, draw_list_idx_bytes}, index_buffer_offset)) {
      IM_ASSERT(false && "Could not copy indices to buffer.");
    }

    auto viewport = impeller::Viewport{
        .rect =
            impeller::Rect(draw_data->DisplayPos.x, draw_data->DisplayPos.y,
                           draw_data->DisplaySize.x, draw_data->DisplaySize.y)};

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

      if (pcmd->UserCallback) {
        pcmd->UserCallback(cmd_list, pcmd);
      } else {
        // Project scissor/clipping rectangles into framebuffer space.
        impeller::IPoint clip_min(pcmd->ClipRect.x - draw_data->DisplayPos.x,
                                  pcmd->ClipRect.y - draw_data->DisplayPos.y);
        impeller::IPoint clip_max(pcmd->ClipRect.z - draw_data->DisplayPos.x,
                                  pcmd->ClipRect.w - draw_data->DisplayPos.y);
        // Ensure the scissor never goes out of bounds.
        clip_min.x = std::clamp(
            clip_min.x, 0ll,
            static_cast<decltype(clip_min.x)>(draw_data->DisplaySize.x));
        clip_min.y = std::clamp(
            clip_min.y, 0ll,
            static_cast<decltype(clip_min.y)>(draw_data->DisplaySize.y));
        if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) {
          continue;  // Nothing to render.
        }

        impeller::Command cmd;
        cmd.label = impeller::SPrintF("ImGui draw list %d (command %d)",
                                      draw_list_i, cmd_i);

        cmd.viewport = viewport;
        cmd.scissor = impeller::IRect::MakeLTRB(
            std::max(0ll, clip_min.x), std::max(0ll, clip_min.y),
            std::min(render_pass.GetRenderTargetSize().width, clip_max.x),
            std::min(render_pass.GetRenderTargetSize().height, clip_max.y));

        cmd.winding = impeller::WindingOrder::kClockwise;
        cmd.pipeline = bd->pipeline;
        VS::BindUniformBuffer(
            cmd, render_pass.GetTransientsBuffer().EmplaceUniform(uniforms));
        FS::BindTex(cmd, bd->font_texture, bd->sampler);

        size_t vb_start =
            vertex_buffer_offset + pcmd->VtxOffset * sizeof(ImDrawVert);

        impeller::VertexBuffer vertex_buffer;
        vertex_buffer.vertex_buffer = {
            .buffer = buffer,
            .range = impeller::Range(vb_start, draw_list_vtx_bytes - vb_start)};
        vertex_buffer.index_buffer = {
            .buffer = buffer,
            .range = impeller::Range(
                index_buffer_offset + pcmd->IdxOffset * sizeof(ImDrawIdx),
                pcmd->ElemCount * sizeof(ImDrawIdx))};
        vertex_buffer.index_count = pcmd->ElemCount;
        vertex_buffer.index_type = impeller::IndexType::k16bit;
        cmd.BindVertices(vertex_buffer);
        cmd.base_vertex = pcmd->VtxOffset;
        cmd.primitive_type = impeller::PrimitiveType::kTriangle;

        render_pass.AddCommand(std::move(cmd));
      }
    }

    vertex_buffer_offset += draw_list_vtx_bytes;
    index_buffer_offset += draw_list_idx_bytes;
  }
}
