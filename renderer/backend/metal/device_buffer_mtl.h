// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <Metal/Metal.h>

#include "flutter/fml/macros.h"
#include "impeller/base/backend_cast.h"
#include "impeller/renderer/device_buffer.h"

namespace impeller {

class DeviceBufferMTL final
    : public DeviceBuffer,
      public BackendCast<DeviceBufferMTL, DeviceBuffer> {
 public:
  DeviceBufferMTL();

  // |DeviceBuffer|
  ~DeviceBufferMTL() override;

  id<MTLBuffer> GetMTLBuffer() const;

 private:
  friend class AllocatorMTL;

  const id<MTLBuffer> buffer_;
  const size_t size_;
  const StorageMode mode_;

  DeviceBufferMTL(id<MTLBuffer> buffer, size_t size, StorageMode mode);

  // |DeviceBuffer|
  bool CopyHostBuffer(const uint8_t* source,
                      Range source_range,
                      size_t offset) override;

  // |DeviceBuffer|
  std::shared_ptr<Texture> MakeTexture(TextureDescriptor desc,
                                       size_t offset) const override;

  // |DeviceBuffer|
  bool SetLabel(const std::string& label) override;

  // |DeviceBuffer|
  bool SetLabel(const std::string& label, Range range) override;

  // |DeviceBuffer|
  BufferView AsBufferView() const override;

  // |Buffer|
  std::shared_ptr<const DeviceBuffer> GetDeviceBuffer(
      Allocator& allocator) const override;

  FML_DISALLOW_COPY_AND_ASSIGN(DeviceBufferMTL);
};

}  // namespace impeller
