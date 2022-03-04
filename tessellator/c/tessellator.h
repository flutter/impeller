// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "impeller/geometry/path_builder.h"
#include "impeller/tessellator/tessellator.h"

#define IMPELLER_API __attribute__((visibility("default")))

extern "C" {

namespace impeller {

struct IMPELLER_API Vertices {
  float* points;
  uint32_t length;
};

IMPELLER_API PathBuilder* CreatePathBuilder();

IMPELLER_API void DestroyPathBuilder(PathBuilder* builder);

IMPELLER_API void MoveTo(PathBuilder* builder, Scalar x, Scalar y);

IMPELLER_API void LineTo(PathBuilder* builder, Scalar x, Scalar y);

IMPELLER_API void CubicTo(PathBuilder* builder,
                          Scalar x1,
                          Scalar y1,
                          Scalar x2,
                          Scalar y2,
                          Scalar x3,
                          Scalar y3);

IMPELLER_API void Close(PathBuilder* builder);

IMPELLER_API struct Vertices* Tessellate(PathBuilder* builder);

IMPELLER_API void DestroyVertices(Vertices* vertices);

}  // namespace impeller
}
