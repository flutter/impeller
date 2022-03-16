// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

uniform sampler2D texture_sampler;

in vec2 v_texture_coords;
in float v_blur_radius;

out vec4 frag_color;

vec4 SampleColor() {
  // TODO(bdero): Add optional support for SamplerAddressMode::ClampToBorder to
  // avoid this branch on most targets.
  if (v_texture_coords.x > 0 && v_texture_coords.y > 0 &&
      v_texture_coords.x < 1 && v_texture_coords.y < 1) {
    vec4 sampled = texture(texture_sampler, v_texture_coords);
    return vec4(1, sampled.gba);
  }

  return vec4(0);
}

void main() {
  frag_color = SampleColor();
}
