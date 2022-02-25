// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/metal/sampler_library_mtl.h"

#include "impeller/renderer/backend/metal/formats_mtl.h"
#include "impeller/renderer/backend/metal/sampler_mtl.h"

namespace impeller {

SamplerLibraryMTL::SamplerLibraryMTL(id<MTLDevice> device) : device_(device) {}

SamplerLibraryMTL::~SamplerLibraryMTL() = default;

std::shared_ptr<const Sampler> SamplerLibraryMTL::GetSampler(
    SamplerDescriptor descriptor) {
  auto found = samplers_.find(descriptor);
  if (found != samplers_.end()) {
    return found->second;
  }
  if (!device_) {
    return nullptr;
  }
  auto desc = [[MTLSamplerDescriptor alloc] init];
  desc.minFilter = ToMTLSamplerMinMagFilter(descriptor.min_filter);
  desc.magFilter = ToMTLSamplerMinMagFilter(descriptor.mag_filter);
  desc.sAddressMode = MTLSamplerAddressMode(descriptor.width_address_mode);
  desc.rAddressMode = MTLSamplerAddressMode(descriptor.depth_address_mode);
  desc.tAddressMode = MTLSamplerAddressMode(descriptor.height_address_mode);

  if (!descriptor.label.empty()) {
    desc.label = @(descriptor.label.c_str());
  }

  auto mtl_sampler = [device_ newSamplerStateWithDescriptor:desc];
  if (!mtl_sampler) {
    return nullptr;
  }
  auto sampler = std::shared_ptr<SamplerMTL>(new SamplerMTL(mtl_sampler));
  if (!sampler->IsValid()) {
    return nullptr;
  }
  samplers_[descriptor] = sampler;
  return sampler;
}

}  // namespace impeller
