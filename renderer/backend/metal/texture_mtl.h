// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <Metal/Metal.h>

#include "flutter/fml/macros.h"
#include "impeller/base/backend_cast.h"
#include "impeller/renderer/texture.h"

namespace impeller {

class TextureMTL final : public Texture,
                         public BackendCast<TextureMTL, Texture> {
 public:
  TextureMTL(TextureDescriptor desc, id<MTLTexture> texture);

  // |Texture|
  ~TextureMTL() override;

  // |Texture|
  void SetLabel(const std::string_view& label) override;

  // |Texture|
  bool SetContents(const uint8_t* contents, size_t length) override;

  // |Texture|
  bool IsValid() const override;

  // |Texture|
  ISize GetSize() const override;

  id<MTLTexture> GetMTLTexture() const;

 private:
  id<MTLTexture> texture_ = nullptr;
  bool is_valid_ = false;

  FML_DISALLOW_COPY_AND_ASSIGN(TextureMTL);
};

}  // namespace impeller
