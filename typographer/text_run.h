// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <vector>

#include "impeller/geometry/matrix.h"
#include "impeller/typographer/font.h"
#include "impeller/typographer/glyph.h"

namespace impeller {

//------------------------------------------------------------------------------
/// @brief      Represents a collection of positioned glyphs from a specific
///             font.
///
class TextRun {
 public:
  struct GlyphPosition {
    Glyph glyph;
    Matrix position;

    GlyphPosition(Glyph p_glyph, Matrix p_position)
        : glyph(p_glyph), position(p_position) {}
  };

  //----------------------------------------------------------------------------
  /// @brief      Create an empty text run with the specified font.
  ///
  /// @param[in]  font  The font
  ///
  TextRun(Font font);

  ~TextRun();

  bool IsValid() const;

  //----------------------------------------------------------------------------
  /// @brief      Add a glyph at the specified position to the run.
  ///
  /// @param[in]  glyph     The glyph
  /// @param[in]  position  The position
  ///
  /// @return     If the glyph could be added to the run.
  ///
  bool AddGlyph(Glyph glyph, Matrix position);

  //----------------------------------------------------------------------------
  /// @brief      Get the number of glyphs in the run.
  ///
  /// @return     The glyph count.
  ///
  size_t GetGlyphCount() const;

  //----------------------------------------------------------------------------
  /// @brief      Get a reference to all the glyph positions within the run.
  ///
  /// @return     The glyph positions.
  ///
  const std::vector<GlyphPosition>& GetGlyphPositions() const;

  //----------------------------------------------------------------------------
  /// @brief      Get the font for this run.
  ///
  /// @return     The font.
  ///
  const Font& GetFont() const;

 private:
  Font font_;
  std::vector<GlyphPosition> glyphs_;
  bool is_valid_ = false;
};

}  // namespace impeller
