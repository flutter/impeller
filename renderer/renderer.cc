// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/renderer.h"

#include <algorithm>

#include "flutter/fml/logging.h"
#include "flutter/fml/trace_event.h"
#include "impeller/base/validation.h"
#include "impeller/renderer/command_buffer.h"
#include "impeller/renderer/surface.h"

namespace impeller {

Renderer::Renderer(std::shared_ptr<Context> context,
                   size_t max_frames_in_flight)
    : frames_in_flight_sema_(std::make_shared<fml::Semaphore>(
          std::max<std::size_t>(1u, max_frames_in_flight))),
      context_(std::move(context)) {
  if (!context_->IsValid()) {
    return;
  }

  is_valid_ = true;
}

Renderer::~Renderer() = default;

bool Renderer::IsValid() const {
  return is_valid_;
}

bool Renderer::Render(std::unique_ptr<Surface> surface,
                      RenderCallback render_callback) const {
  TRACE_EVENT0("impeller", "Renderer::Render");
  if (!IsValid()) {
    return false;
  }

  if (!surface || !surface->IsValid()) {
    return false;
  }

  auto command_buffer = context_->CreateRenderCommandBuffer();

  if (!command_buffer) {
    return false;
  }

  command_buffer->SetLabel("Onscreen Command Buffer");

  auto render_pass = command_buffer->CreateRenderPass(
      surface->GetTargetRenderPassDescriptor());
  if (!render_pass) {
    return false;
  }

  render_pass->SetLabel("Onscreen Render Pass");

  if (render_callback && !render_callback(*render_pass)) {
    return false;
  }

  if (!render_pass->EncodeCommands(*GetContext()->GetTransientsAllocator())) {
    return false;
  }

  if (!frames_in_flight_sema_->Wait()) {
    return false;
  }

  if (!command_buffer->SubmitCommands(
          [sema = frames_in_flight_sema_](CommandBuffer::Status result) {
            sema->Signal();
            if (result != CommandBuffer::Status::kCompleted) {
              VALIDATION_LOG << "Could not commit command buffer.";
            }
          })) {
    return false;
  }

  return surface->Present();
}

std::shared_ptr<Context> Renderer::GetContext() const {
  return context_;
}

}  // namespace impeller
