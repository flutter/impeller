// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <limits>
#include <memory>

#include "flutter/fml/macros.h"

namespace impeller {

class Allocation {
 public:
  Allocation();

  ~Allocation();

  uint8_t* GetBuffer() const;

  size_t GetLength() const;

  size_t GetReservedLength() const;

  [[nodiscard]] bool Truncate(size_t length, bool npot = true);

  static uint32_t NextPowerOfTwoSize(uint32_t x);

 private:
  uint8_t* buffer_ = nullptr;
  size_t length_ = 0;
  size_t reserved_ = 0;

  [[nodiscard]] bool Reserve(size_t reserved);

  [[nodiscard]] bool ReserveNPOT(size_t reserved);

  FML_DISALLOW_COPY_AND_ASSIGN(Allocation);
};

}  // namespace impeller
