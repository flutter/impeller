// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/base/allocation.h"

#include <algorithm>

#include "flutter/fml/logging.h"
#include "impeller/base/validation.h"

namespace impeller {

Allocation::Allocation() = default;

Allocation::~Allocation() {
  ::free(buffer_);
}

uint8_t* Allocation::GetBuffer() const {
  return buffer_;
}

size_t Allocation::GetLength() const {
  return length_;
}

size_t Allocation::GetReservedLength() const {
  return reserved_;
}

bool Allocation::Truncate(size_t length, bool npot) {
  const auto reserved = npot ? ReserveNPOT(length) : Reserve(length);
  if (!reserved) {
    return false;
  }
  length_ = length;
  return true;
}

uint32_t Allocation::NextPowerOfTwoSize(uint32_t x) {
  if (x == 0) {
    return 1;
  }

  --x;

  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;

  return x + 1;
}

bool Allocation::ReserveNPOT(size_t reserved) {
  // Reserve at least one page of data.
  reserved = std::max<size_t>(4096u, reserved);
  return Reserve(NextPowerOfTwoSize(reserved));
}

bool Allocation::Reserve(size_t reserved) {
  if (reserved == reserved_) {
    return true;
  }

  auto new_allocation = ::realloc(buffer_, reserved);
  if (!new_allocation) {
    // If new length is zero, a minimum non-zero sized allocation is returned.
    // So this check will not trip and this routine will indicate success as
    // expected.
    VALIDATION_LOG << "Allocation failed. Out of host memory.";
    return false;
  }

  buffer_ = static_cast<uint8_t*>(new_allocation);
  reserved_ = reserved;

  return true;
}

}  // namespace impeller
