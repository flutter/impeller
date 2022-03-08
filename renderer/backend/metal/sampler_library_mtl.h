// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <Metal/Metal.h>

#include <memory>
#include <unordered_map>

#include "flutter/fml/macros.h"
#include "impeller/base/backend_cast.h"
#include "impeller/base/comparable.h"
#include "impeller/renderer/sampler_descriptor.h"
#include "impeller/renderer/sampler_library.h"

namespace impeller {

class SamplerLibraryMTL final
    : public SamplerLibrary,
      public BackendCast<SamplerLibraryMTL, SamplerLibrary> {
 public:
  // |SamplerLibrary|
  ~SamplerLibraryMTL() override;

 private:
  friend class ContextMTL;

  using CachedSamplers = std::unordered_map<SamplerDescriptor,
                                            std::shared_ptr<const Sampler>,
                                            ComparableHash<SamplerDescriptor>,
                                            ComparableEqual<SamplerDescriptor>>;
  id<MTLDevice> device_ = nullptr;
  CachedSamplers samplers_;

  SamplerLibraryMTL(id<MTLDevice> device);

  // |SamplerLibrary|
  std::shared_ptr<const Sampler> GetSampler(
      SamplerDescriptor descriptor) override;

  FML_DISALLOW_COPY_AND_ASSIGN(SamplerLibraryMTL);
};

}  // namespace impeller
