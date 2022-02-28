// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/renderer/backend/metal/device_buffer_mtl.h"

#include "flutter/fml/logging.h"
#include "impeller/base/validation.h"
#include "impeller/renderer/backend/metal/formats_mtl.h"
#include "impeller/renderer/backend/metal/texture_mtl.h"

namespace impeller {

DeviceBufferMTL::DeviceBufferMTL(id<MTLBuffer> buffer,
                                 size_t size,
                                 StorageMode mode)
    : buffer_(buffer), size_(size), mode_(mode) {}

DeviceBufferMTL::~DeviceBufferMTL() = default;

id<MTLBuffer> DeviceBufferMTL::GetMTLBuffer() const {
  return buffer_;
}

std::shared_ptr<Texture> DeviceBufferMTL::MakeTexture(TextureDescriptor desc,
                                                      size_t offset) const {
  if (!desc.IsValid() || !buffer_) {
    return nullptr;
  }

  // Avoid overruns.
  if (offset + desc.GetSizeOfBaseMipLevel() > size_) {
    VALIDATION_LOG << "Avoiding buffer overrun when creating texture.";
    return nullptr;
  }

  if (@available(macOS 10.13, *)) {
    auto texture =
        [buffer_ newTextureWithDescriptor:ToMTLTextureDescriptor(desc)
                                   offset:offset
                              bytesPerRow:desc.GetBytesPerRow()];
    if (!texture) {
      return nullptr;
    }

    return std::make_shared<TextureMTL>(desc, texture);
  } else {
    return nullptr;
  }
}

[[nodiscard]] bool DeviceBufferMTL::CopyHostBuffer(const uint8_t* source,
                                                   Range source_range,
                                                   size_t offset) {
  if (mode_ != StorageMode::kHostVisible) {
    // One of the storage modes where a transfer queue must be used.
    return false;
  }

  if (offset + source_range.length > size_) {
    // Out of bounds of this buffer.
    return false;
  }

  auto dest = static_cast<uint8_t*>(buffer_.contents);

  if (!dest) {
    return false;
  }

  if (source) {
    ::memmove(dest + offset, source + source_range.offset, source_range.length);
  }

// |RequiresExplicitHostSynchronization| always returns false on iOS. But the
// compiler is mad that `didModifyRange:` appears in a TU meant for iOS. So,
// just compile it away.
//
// Making this call is never necessary on iOS because there is no
// MTLResourceStorageModeManaged mode. Only the MTLStorageModeShared mode is
// available.
#if !FML_OS_IOS
  if (Allocator::RequiresExplicitHostSynchronization(mode_)) {
    [buffer_ didModifyRange:NSMakeRange(offset, source_range.length)];
  }
#endif

  return true;
}

// |Buffer|
std::shared_ptr<const DeviceBuffer> DeviceBufferMTL::GetDeviceBuffer(
    Allocator& allocator) const {
  return shared_from_this();
}

bool DeviceBufferMTL::SetLabel(const std::string& label) {
  if (label.empty()) {
    return false;
  }
  [buffer_ setLabel:@(label.c_str())];
  return true;
}

bool DeviceBufferMTL::SetLabel(const std::string& label, Range range) {
  if (label.empty()) {
    return false;
  }
  if (@available(macOS 10.12, iOS 10.0, *)) {
    [buffer_ addDebugMarker:@(label.c_str())
                      range:NSMakeRange(range.offset, range.length)];
  }
  return true;
}

BufferView DeviceBufferMTL::AsBufferView() const {
  BufferView view;
  view.buffer = shared_from_this();
  view.range = {0u, size_};
  return view;
}

}  // namespace impeller
