// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/entity/contents/contents.h"

#include <memory>
#include <tuple>

#include "flutter/fml/logging.h"
#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/entity.h"
#include "impeller/geometry/path_builder.h"
#include "impeller/geometry/path_component.h"
#include "impeller/geometry/scalar.h"
#include "impeller/geometry/vector.h"
#include "impeller/renderer/render_pass.h"
#include "impeller/renderer/sampler_library.h"
#include "impeller/renderer/surface.h"
#include "impeller/renderer/tessellator.h"
#include "impeller/renderer/vertex_buffer.h"
#include "impeller/renderer/vertex_buffer_builder.h"

namespace impeller {

ContentContextOptions OptionsFromPass(const RenderPass& pass) {
  ContentContextOptions opts;
  opts.sample_count = pass.GetRenderTarget().GetSampleCount();
  return opts;
}

Contents::Contents() = default;

Contents::~Contents() = default;

}  // namespace impeller
