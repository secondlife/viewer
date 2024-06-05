/**
 * @file llfontfreetypesvg.h
 * @brief Freetype font library SVG glyph rendering
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#pragma once

#include <ft2build.h>
#include FT_TYPES_H
#include FT_MODULE_H
#include FT_OTSVG_H

 // See https://freetype.org/freetype2/docs/reference/ft2-svg_fonts.html
class LLFontFreeTypeSvgRenderer
{
public:
    // Called when the very first OT-SVG glyph is rendered (across the entire lifetime of our FT_Library object)
    static FT_Error OnInit(FT_Pointer* state);

    // Called when the ot-svg module is being freed (but only called if the init hook was called previously)
    static void     OnFree(FT_Pointer* state);

    // Called to preset the glyph slot, twice per glyph:
    //   - when FT_Load_Glyph needs to preset the glyph slot (with cache == false)
    //   - right before the svg module calls the render callback hook. (with cache == true)
    static FT_Error OnPresetGlypthSlot(FT_GlyphSlot glyph_slot, FT_Bool cache, FT_Pointer* state);

    // Called to render an OT-SVG glyph (right after the preset hook OnPresetGlypthSlot was called with cache set to true)
    static FT_Error OnRender(FT_GlyphSlot glyph_slot, FT_Pointer* state);

    // Called to deallocate our per glyph slot data
    static void OnDataFinalizer(void* objectp);
};
