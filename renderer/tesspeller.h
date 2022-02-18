// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "impeller/geometry/path_builder.h"
#include "impeller/renderer/tessellator.h"

#define TESSPELLER_API __attribute__((visibility("default")))

extern "C" {

namespace impeller {

struct TESSPELLER_API Vertices {
  float* points;
  uint32_t length;
};

TESSPELLER_API PathBuilder* CreatePathBuilder();

TESSPELLER_API void DestroyPathBuilder(PathBuilder* builder);

TESSPELLER_API void MoveTo(PathBuilder* builder, Scalar x, Scalar y);

TESSPELLER_API void LineTo(PathBuilder* builder, Scalar x, Scalar y);

TESSPELLER_API void CubicTo(PathBuilder* builder,
                            Scalar x1,
                            Scalar y1,
                            Scalar x2,
                            Scalar y2,
                            Scalar x3,
                            Scalar y3);

TESSPELLER_API void Close(PathBuilder* builder);

TESSPELLER_API void AddRect(PathBuilder* builder,
                            Scalar left,
                            Scalar top,
                            Scalar right,
                            Scalar bottom);

TESSPELLER_API void AddRoundedRect(PathBuilder* builder,
                                   Scalar left,
                                   Scalar top,
                                   Scalar right,
                                   Scalar bottom,
                                   Scalar rx,
                                   Scalar ry);

TESSPELLER_API void AddOval(PathBuilder* builder,
                            Scalar left,
                            Scalar top,
                            Scalar right,
                            Scalar bottom);

TESSPELLER_API struct Vertices* Tessellate(PathBuilder* builder);

TESSPELLER_API void DestroyVertices(Vertices* vertices);

}  // namespace impeller
}
