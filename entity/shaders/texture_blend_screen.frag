// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

uniform sampler2D texture_sampler_d;
uniform sampler2D texture_sampler_s;

in vec2 v_texture_coords;

out vec4 frag_color;

void main() {
  vec4 d = texture(texture_sampler_d, v_texture_coords);
  vec4 s = texture(texture_sampler_s, v_texture_coords);
  frag_color = s + d - s * d;
}
