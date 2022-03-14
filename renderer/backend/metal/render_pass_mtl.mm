// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/metal/render_pass_mtl.h"

#include "flutter/fml/closure.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/trace_event.h"
#include "impeller/base/base.h"
#include "impeller/renderer/backend/metal/device_buffer_mtl.h"
#include "impeller/renderer/backend/metal/formats_mtl.h"
#include "impeller/renderer/backend/metal/pipeline_mtl.h"
#include "impeller/renderer/backend/metal/sampler_mtl.h"
#include "impeller/renderer/backend/metal/texture_mtl.h"
#include "impeller/renderer/formats.h"
#include "impeller/renderer/host_buffer.h"
#include "impeller/renderer/shader_types.h"

namespace impeller {

static bool ConfigureResolveTextureAttachment(
    const Attachment& desc,
    MTLRenderPassAttachmentDescriptor* attachment) {
  if (desc.store_action == StoreAction::kMultisampleResolve &&
      !desc.resolve_texture) {
    VALIDATION_LOG << "Resolve store action specified on attachment but no "
                      "resolve texture was specified.";
    return false;
  }

  if (desc.resolve_texture &&
      desc.store_action != StoreAction::kMultisampleResolve) {
    VALIDATION_LOG << "Resolve store action specified but there was no "
                      "resolve attachment.";
    return false;
  }

  if (!desc.resolve_texture) {
    return true;
  }

  attachment.resolveTexture =
      TextureMTL::Cast(*desc.resolve_texture).GetMTLTexture();

  return true;
}

static bool ConfigureAttachment(const Attachment& desc,
                                MTLRenderPassAttachmentDescriptor* attachment) {
  if (!desc.texture) {
    return false;
  }

  attachment.texture = TextureMTL::Cast(*desc.texture).GetMTLTexture();
  attachment.loadAction = ToMTLLoadAction(desc.load_action);
  attachment.storeAction = ToMTLStoreAction(desc.store_action);

  if (!ConfigureResolveTextureAttachment(desc, attachment)) {
    return false;
  }

  return true;
}

static bool ConfigureColorAttachment(
    const ColorAttachment& desc,
    MTLRenderPassColorAttachmentDescriptor* attachment) {
  if (!ConfigureAttachment(desc, attachment)) {
    return false;
  }
  attachment.clearColor = ToMTLClearColor(desc.clear_color);
  return true;
}

static bool ConfigureDepthAttachment(
    const DepthAttachment& desc,
    MTLRenderPassDepthAttachmentDescriptor* attachment) {
  if (!ConfigureAttachment(desc, attachment)) {
    return false;
  }
  attachment.clearDepth = desc.clear_depth;
  return true;
}

static bool ConfigureStencilAttachment(
    const StencilAttachment& desc,
    MTLRenderPassStencilAttachmentDescriptor* attachment) {
  if (!ConfigureAttachment(desc, attachment)) {
    return false;
  }
  attachment.clearStencil = desc.clear_stencil;
  return true;
}

// TODO(csg): Move this to formats_mtl.h
static MTLRenderPassDescriptor* ToMTLRenderPassDescriptor(
    const RenderTarget& desc) {
  auto result = [MTLRenderPassDescriptor renderPassDescriptor];

  const auto& colors = desc.GetColorAttachments();

  for (const auto& color : colors) {
    if (!ConfigureColorAttachment(color.second,
                                  result.colorAttachments[color.first])) {
      VALIDATION_LOG << "Could not configure color attachment at index "
                     << color.first;
      return nil;
    }
  }

  const auto& depth = desc.GetDepthAttachment();

  if (depth.has_value() &&
      !ConfigureDepthAttachment(depth.value(), result.depthAttachment)) {
    VALIDATION_LOG << "Could not configure depth attachment.";
    return nil;
  }

  const auto& stencil = desc.GetStencilAttachment();

  if (stencil.has_value() &&
      !ConfigureStencilAttachment(stencil.value(), result.stencilAttachment)) {
    VALIDATION_LOG << "Could not configure stencil attachment.";
    return nil;
  }

  return result;
}

RenderPassMTL::RenderPassMTL(id<MTLCommandBuffer> buffer, RenderTarget target)
    : RenderPass(std::move(target)),
      buffer_(buffer),
      desc_(ToMTLRenderPassDescriptor(GetRenderTarget())),
      transients_buffer_(HostBuffer::Create()) {
  if (!buffer_ || !desc_ || !render_target_.IsValid()) {
    return;
  }
  SetLabel("RenderPass");
  is_valid_ = true;
}

RenderPassMTL::~RenderPassMTL() = default;

HostBuffer& RenderPassMTL::GetTransientsBuffer() {
  return *transients_buffer_;
}

bool RenderPassMTL::IsValid() const {
  return is_valid_;
}

void RenderPassMTL::SetLabel(std::string label) {
  if (label.empty()) {
    return;
  }
  label_ = std::move(label);
  transients_buffer_->SetLabel(SPrintF("%s Transients", label_.c_str()));
}

bool RenderPassMTL::EncodeCommands(Allocator& transients_allocator) const {
  TRACE_EVENT0("impeller", "RenderPassMTL::EncodeCommands");
  if (!IsValid()) {
    return false;
  }
  auto render_command_encoder =
      [buffer_ renderCommandEncoderWithDescriptor:desc_];

  if (!render_command_encoder) {
    return false;
  }

  if (!label_.empty()) {
    [render_command_encoder setLabel:@(label_.c_str())];
  }

  // Success or failure, the pass must end. The buffer can only process one pass
  // at a time.
  fml::ScopedCleanupClosure auto_end(
      [render_command_encoder]() { [render_command_encoder endEncoding]; });

  return EncodeCommands(transients_allocator, render_command_encoder);
}

//-----------------------------------------------------------------------------
/// @brief      Ensures that bindings on the pass are not redundantly set or
///             updated. Avoids making the driver do additional checks and makes
///             the frame insights during profiling and instrumentation not
///             complain about the same.
///
///             There should be no change to rendering if this caching was
///             absent.
///
struct PassBindingsCache {
  PassBindingsCache(id<MTLRenderCommandEncoder> encoder) : encoder_(encoder) {}

  PassBindingsCache(const PassBindingsCache&) = delete;

  PassBindingsCache(PassBindingsCache&&) = delete;

  void SetRenderPipelineState(id<MTLRenderPipelineState> pipeline) {
    if (pipeline == pipeline_) {
      return;
    }
    pipeline_ = pipeline;
    [encoder_ setRenderPipelineState:pipeline_];
  }

  void SetDepthStencilState(id<MTLDepthStencilState> depth_stencil) {
    if (depth_stencil_ == depth_stencil) {
      return;
    }
    depth_stencil_ = depth_stencil;
    [encoder_ setDepthStencilState:depth_stencil_];
  }

  bool SetBuffer(ShaderStage stage,
                 uint64_t index,
                 uint64_t offset,
                 id<MTLBuffer> buffer) {
    auto& buffers_map = buffers_[stage];
    auto found = buffers_map.find(index);
    if (found != buffers_map.end() && found->second.buffer == buffer) {
      // The right buffer is bound. Check if its offset needs to be updated.
      if (found->second.offset == offset) {
        // Buffer and its offset is identical. Nothing to do.
        return true;
      }

      // Only the offset needs to be updated.
      found->second.offset = offset;

      switch (stage) {
        case ShaderStage::kVertex:
          [encoder_ setVertexBufferOffset:offset atIndex:index];
          return true;
        case ShaderStage::kFragment:
          [encoder_ setFragmentBufferOffset:offset atIndex:index];
          return true;
        default:
          VALIDATION_LOG << "Cannot update buffer offset of an unknown stage.";
          return false;
      }
      return true;
    }
    buffers_map[index] = {buffer, offset};
    switch (stage) {
      case ShaderStage::kVertex:
        [encoder_ setVertexBuffer:buffer offset:offset atIndex:index];
        return true;
      case ShaderStage::kFragment:
        [encoder_ setFragmentBuffer:buffer offset:offset atIndex:index];
        return true;
      default:
        VALIDATION_LOG << "Cannot bind buffer to unknown shader stage.";
        return false;
    }
    return false;
  }

  bool SetTexture(ShaderStage stage, uint64_t index, id<MTLTexture> texture) {
    auto& texture_map = textures_[stage];
    auto found = texture_map.find(index);
    if (found != texture_map.end() && found->second == texture) {
      // Already bound.
      return true;
    }
    texture_map[index] = texture;
    switch (stage) {
      case ShaderStage::kVertex:
        [encoder_ setVertexTexture:texture atIndex:index];
        return true;
      case ShaderStage::kFragment:
        [encoder_ setFragmentTexture:texture atIndex:index];
        return true;
      default:
        VALIDATION_LOG << "Cannot bind buffer to unknown shader stage.";
        return false;
    }
    return false;
  }

  bool SetSampler(ShaderStage stage,
                  uint64_t index,
                  id<MTLSamplerState> sampler) {
    auto& sampler_map = samplers_[stage];
    auto found = sampler_map.find(index);
    if (found != sampler_map.end() && found->second == sampler) {
      // Already bound.
      return true;
    }
    sampler_map[index] = sampler;
    switch (stage) {
      case ShaderStage::kVertex:
        [encoder_ setVertexSamplerState:sampler atIndex:index];
        return true;
      case ShaderStage::kFragment:
        [encoder_ setFragmentSamplerState:sampler atIndex:index];
        return true;
      default:
        VALIDATION_LOG << "Cannot bind buffer to unknown shader stage.";
        return false;
    }
    return false;
  }

 private:
  struct BufferOffsetPair {
    id<MTLBuffer> buffer = nullptr;
    size_t offset = 0u;
  };
  using BufferMap = std::map<uint64_t, BufferOffsetPair>;
  using TextureMap = std::map<uint64_t, id<MTLTexture>>;
  using SamplerMap = std::map<uint64_t, id<MTLSamplerState>>;

  const id<MTLRenderCommandEncoder> encoder_;
  id<MTLRenderPipelineState> pipeline_ = nullptr;
  id<MTLDepthStencilState> depth_stencil_ = nullptr;
  std::map<ShaderStage, BufferMap> buffers_;
  std::map<ShaderStage, TextureMap> textures_;
  std::map<ShaderStage, SamplerMap> samplers_;
};

static bool Bind(PassBindingsCache& pass,
                 Allocator& allocator,
                 ShaderStage stage,
                 size_t bind_index,
                 const BufferView& view) {
  if (!view.buffer) {
    return false;
  }

  auto device_buffer = view.buffer->GetDeviceBuffer(allocator);
  if (!device_buffer) {
    return false;
  }

  auto buffer = DeviceBufferMTL::Cast(*device_buffer).GetMTLBuffer();
  // The Metal call is a void return and we don't want to make it on nil.
  if (!buffer) {
    return false;
  }

  return pass.SetBuffer(stage, bind_index, view.range.offset, buffer);
}

static bool Bind(PassBindingsCache& pass,
                 ShaderStage stage,
                 size_t bind_index,
                 const Texture& texture) {
  if (!texture.IsValid()) {
    return false;
  }

  return pass.SetTexture(stage, bind_index,
                         TextureMTL::Cast(texture).GetMTLTexture());
}

static bool Bind(PassBindingsCache& pass,
                 ShaderStage stage,
                 size_t bind_index,
                 const Sampler& sampler) {
  if (!sampler.IsValid()) {
    return false;
  }

  return pass.SetSampler(stage, bind_index,
                         SamplerMTL::Cast(sampler).GetMTLSamplerState());
}

bool RenderPassMTL::EncodeCommands(Allocator& allocator,
                                   id<MTLRenderCommandEncoder> encoder) const {
  PassBindingsCache pass_bindings(encoder);
  auto bind_stage_resources = [&allocator, &pass_bindings](
                                  const Bindings& bindings,
                                  ShaderStage stage) -> bool {
    for (const auto& buffer : bindings.buffers) {
      if (!Bind(pass_bindings, allocator, stage, buffer.first, buffer.second)) {
        return false;
      }
    }
    for (const auto& texture : bindings.textures) {
      if (!Bind(pass_bindings, stage, texture.first, *texture.second)) {
        return false;
      }
    }
    for (const auto& sampler : bindings.samplers) {
      if (!Bind(pass_bindings, stage, sampler.first, *sampler.second)) {
        return false;
      }
    }
    return true;
  };

  const auto target_sample_count = render_target_.GetSampleCount();

  fml::closure pop_debug_marker = [encoder]() { [encoder popDebugGroup]; };
  for (const auto& command : commands_) {
    if (command.index_count == 0u) {
      continue;
    }

    fml::ScopedCleanupClosure auto_pop_debug_marker(pop_debug_marker);
    if (!command.label.empty()) {
      [encoder pushDebugGroup:@(command.label.c_str())];
    } else {
      auto_pop_debug_marker.Release();
    }

    if (target_sample_count !=
        command.pipeline->GetDescriptor().GetSampleCount()) {
      VALIDATION_LOG << "Pipeline for command and the render target disagree "
                        "on sample counts (target was "
                     << static_cast<uint64_t>(target_sample_count)
                     << " but pipeline wanted "
                     << static_cast<uint64_t>(
                            command.pipeline->GetDescriptor().GetSampleCount())
                     << ").";
      return false;
    }

    pass_bindings.SetRenderPipelineState(
        PipelineMTL::Cast(*command.pipeline).GetMTLRenderPipelineState());
    pass_bindings.SetDepthStencilState(
        PipelineMTL::Cast(*command.pipeline).GetMTLDepthStencilState());
    [encoder setFrontFacingWinding:command.winding == WindingOrder::kClockwise
                                       ? MTLWindingClockwise
                                       : MTLWindingCounterClockwise];
    [encoder setCullMode:ToMTLCullMode(command.cull_mode)];
    [encoder setStencilReferenceValue:command.stencil_reference];
    if (command.viewport.has_value()) {
      auto v = command.viewport.value();
      MTLViewport viewport = {
          .originX = v.rect.origin.x,
          .originY = v.rect.origin.y,
          .width = v.rect.size.width,
          .height = v.rect.size.height,
          .znear = v.znear,
          .zfar = v.zfar,
      };
      [encoder setViewport:viewport];
    }
    if (command.scissor.has_value()) {
      auto s = command.scissor.value();
      MTLScissorRect scissor = {
          .x = static_cast<NSUInteger>(s.origin.x),
          .y = static_cast<NSUInteger>(s.origin.y),
          .width = static_cast<NSUInteger>(s.size.width),
          .height = static_cast<NSUInteger>(s.size.height),
      };
      [encoder setScissorRect:scissor];
    }
    if (!bind_stage_resources(command.vertex_bindings, ShaderStage::kVertex)) {
      return false;
    }
    if (!bind_stage_resources(command.fragment_bindings,
                              ShaderStage::kFragment)) {
      return false;
    }
    if (command.index_type == IndexType::kUnknown) {
      return false;
    }
    auto index_buffer = command.index_buffer.buffer;
    if (!index_buffer) {
      return false;
    }
    auto device_buffer = index_buffer->GetDeviceBuffer(allocator);
    if (!device_buffer) {
      return false;
    }
    auto mtl_index_buffer =
        DeviceBufferMTL::Cast(*device_buffer).GetMTLBuffer();
    if (!mtl_index_buffer) {
      return false;
    }
    FML_DCHECK(command.index_count *
                   (command.index_type == IndexType::k16bit ? 2 : 4) ==
               command.index_buffer.range.length);
    // Returns void. All error checking must be done by this point.
    [encoder drawIndexedPrimitives:ToMTLPrimitiveType(command.primitive_type)
                        indexCount:command.index_count
                         indexType:ToMTLIndexType(command.index_type)
                       indexBuffer:mtl_index_buffer
                 indexBufferOffset:command.index_buffer.range.offset
                     instanceCount:1u
                        baseVertex:command.base_vertex
                      baseInstance:0u];
  }
  return true;
}

bool RenderPassMTL::AddCommand(Command command) {
  if (!command) {
    VALIDATION_LOG << "Attempted to add an invalid command to the render pass.";
    return false;
  }

  if (command.scissor.has_value()) {
    auto target_rect = IRect({}, render_target_.GetRenderTargetSize());
    if (!target_rect.Contains(command.scissor.value())) {
      VALIDATION_LOG << "Cannot apply a scissor that lies outside the bounds "
                        "of the render target.";
      return false;
    }
  }

  if (command.index_count == 0u) {
    // Essentially a no-op. Don't record the command but this is not necessary
    // an error either.
    return true;
  }

  commands_.emplace_back(std::move(command));
  return true;
}

}  // namespace impeller
