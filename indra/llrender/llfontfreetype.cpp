/**
 * @file llfontfreetype.cpp
 * @brief Freetype font library wrapper
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

#include "linden_common.h"

#include "llfontfreetype.h"
#include "llfontgl.h"

// Freetype stuff
#include <ft2build.h>
#ifdef LL_WINDOWS
#include <freetype2\freetype\ftsystem.h>
#endif
#include "llfontfreetypesvg.h"

// For some reason, this won't work if it's not wrapped in the ifdef
#ifdef FT_FREETYPE_H
#include FT_FREETYPE_H
#endif

#include "lldir.h"
#include "llerror.h"
#include "llimage.h"
#include "llimagepng.h"
//#include "llimagej2c.h"
#include "llmath.h" // Linden math
#include "llstring.h"
//#include "imdebug.h"
#include "llfontbitmapcache.h"
#include "llgl.h"

#if !defined(LL_NO_OTSVG)
    #define ENABLE_OT_SVG_SUPPORT
#endif

FT_Render_Mode gFontRenderMode = FT_RENDER_MODE_NORMAL;

LLFontManager *gFontManagerp = nullptr;

FT_Library gFTLibrary = nullptr;

//static
void LLFontManager::initClass()
{
    if (!gFontManagerp)
    {
        gFontManagerp = new LLFontManager;
    }
}

//static
void LLFontManager::cleanupClass()
{
    delete gFontManagerp;
    gFontManagerp = nullptr;
}

LLFontManager::LLFontManager()
{
    int error;
    error = FT_Init_FreeType(&gFTLibrary);
    if (error)
    {
        // Clean up freetype libs.
        LL_ERRS() << "Freetype initialization failure!" << LL_ENDL;
        FT_Done_FreeType(gFTLibrary);
    }

#if defined(ENABLE_OT_SVG_SUPPORT)
    SVG_RendererHooks hooks = {
        LLFontFreeTypeSvgRenderer::OnInit,
        LLFontFreeTypeSvgRenderer::OnFree,
        LLFontFreeTypeSvgRenderer::OnRender,
        LLFontFreeTypeSvgRenderer::OnPresetGlypthSlot,
    };
    FT_Property_Set(gFTLibrary, "ot-svg", "svg-hooks", &hooks);
#endif
}

LLFontManager::~LLFontManager()
{
    FT_Done_FreeType(gFTLibrary);
    unloadAllFonts();
}


LLFontGlyphInfo::LLFontGlyphInfo(U32 index, EFontGlyphType glyph_type)
:   mGlyphIndex(index),
    mGlyphType(glyph_type),
    mWidth(0),          // In pixels
    mHeight(0),         // In pixels
    mXAdvance(0.f),     // In pixels
    mYAdvance(0.f),     // In pixels
    mXBitmapOffset(0),  // Offset to the origin in the bitmap
    mYBitmapOffset(0),  // Offset to the origin in the bitmap
    mXBearing(0),       // Distance from baseline to left in pixels
    mYBearing(0),       // Distance from baseline to top in pixels
    mBitmapEntry(std::make_pair(EFontGlyphType::Unspecified, -1)) // Which bitmap in the bitmap cache contains this glyph
{
}

LLFontGlyphInfo::LLFontGlyphInfo(const LLFontGlyphInfo& fgi)
    : mGlyphIndex(fgi.mGlyphIndex)
    , mGlyphType(fgi.mGlyphType)
    , mWidth(fgi.mWidth)
    , mHeight(fgi.mHeight)
    , mXAdvance(fgi.mXAdvance)
    , mYAdvance(fgi.mYAdvance)
    , mXBitmapOffset(fgi.mXBitmapOffset)
    , mYBitmapOffset(fgi.mYBitmapOffset)
    , mXBearing(fgi.mXBearing)
    , mYBearing(fgi.mYBearing)
{
    mBitmapEntry = fgi.mBitmapEntry;
}

LLFontFreetype::LLFontFreetype()
:   mFontBitmapCachep(new LLFontBitmapCache),
    mAscender(0.f),
    mDescender(0.f),
    mLineHeight(0.f),
    mIsFallback(false),
    mFTFace(nullptr),
    mRenderGlyphCount(0),
    mAddGlyphCount(0),
    mStyle(0),
    mPointSize(0)
{
}


LLFontFreetype::~LLFontFreetype()
{
    // Clean up freetype libs.
    if (mFTFace)
        FT_Done_Face(mFTFace);
    mFTFace = nullptr;

    // Delete glyph info
    std::for_each(mCharGlyphInfoMap.begin(), mCharGlyphInfoMap.end(), DeletePairedPointer());
    mCharGlyphInfoMap.clear();

    delete mFontBitmapCachep;
    // mFallbackFonts cleaned up by LLPointer destructor
}

bool LLFontFreetype::loadFace(const std::string& filename, F32 point_size, F32 vert_dpi, F32 horz_dpi, bool is_fallback, S32 face_n)
{
    // Don't leak face objects.  This is also needed to deal with
    // changed font file names.
    if (mFTFace)
    {
        FT_Done_Face(mFTFace);
        mFTFace = nullptr;
    }

    FT_Open_Args openArgs;
    memset( &openArgs, 0, sizeof( openArgs ) );
    openArgs.memory_base = gFontManagerp->loadFont( filename, openArgs.memory_size );

    if( !openArgs.memory_base )
        return false;

    openArgs.flags = FT_OPEN_MEMORY;
    int error = FT_Open_Face( gFTLibrary, &openArgs, 0, &mFTFace );

    if (error)
        return false;

    mIsFallback = is_fallback;
    F32 pixels_per_em = (point_size / 72.f)*vert_dpi; // Size in inches * dpi

    error = FT_Set_Char_Size(mFTFace,    /* handle to face object           */
                            0,       /* char_width in 1/64th of points  */
                            (S32)(point_size*64),   /* char_height in 1/64th of points */
                            (U32)horz_dpi,     /* horizontal device resolution    */
                            (U32)vert_dpi);   /* vertical device resolution      */

    if (error)
    {
        // Clean up freetype libs.
        FT_Done_Face(mFTFace);

        mFTFace = nullptr;
        return false;
    }

    F32 y_max, y_min, x_max, x_min;
    F32 ems_per_unit = 1.f/ mFTFace->units_per_EM;
    F32 pixels_per_unit = pixels_per_em * ems_per_unit;

    // Get size of bbox in pixels
    y_max = mFTFace->bbox.yMax * pixels_per_unit;
    y_min = mFTFace->bbox.yMin * pixels_per_unit;
    x_max = mFTFace->bbox.xMax * pixels_per_unit;
    x_min = mFTFace->bbox.xMin * pixels_per_unit;
    mAscender = mFTFace->ascender * pixels_per_unit;
    mDescender = -mFTFace->descender * pixels_per_unit;
    mLineHeight = mFTFace->height * pixels_per_unit;

    S32 max_char_width = ll_round(0.5f + (x_max - x_min));
    S32 max_char_height = ll_round(0.5f + (y_max - y_min));

    mFontBitmapCachep->init(max_char_width, max_char_height);

    if (!mFTFace->charmap)
    {
        //LL_INFOS() << " no unicode encoding, set whatever encoding there is..." << LL_ENDL;
        FT_Set_Charmap(mFTFace, mFTFace->charmaps[0]);
    }

    if (!mIsFallback)
    {
        // Add the default glyph
        addGlyphFromFont(this, 0, 0, EFontGlyphType::Grayscale);
    }

    mName = filename;
    mPointSize = point_size;

    mStyle = LLFontGL::NORMAL;
    if(mFTFace->style_flags & FT_STYLE_FLAG_BOLD)
    {
        mStyle |= LLFontGL::BOLD;
    }

    if(mFTFace->style_flags & FT_STYLE_FLAG_ITALIC)
    {
        mStyle |= LLFontGL::ITALIC;
    }

    return true;
}

S32 LLFontFreetype::getNumFaces(const std::string& filename)
{
    if (mFTFace)
    {
        FT_Done_Face(mFTFace);
        mFTFace = nullptr;
    }

    S32 num_faces = 1;

    FT_Open_Args openArgs;
    memset( &openArgs, 0, sizeof( openArgs ) );
    openArgs.memory_base = gFontManagerp->loadFont( filename, openArgs.memory_size );
    if( !openArgs.memory_base )
        return 0;
    openArgs.flags = FT_OPEN_MEMORY;
    int error = FT_Open_Face( gFTLibrary, &openArgs, 0, &mFTFace );

    if (error)
        return 0;
    else
        num_faces = mFTFace->num_faces;

    FT_Done_Face(mFTFace);
    mFTFace = nullptr;

    return num_faces;
}

void LLFontFreetype::addFallbackFont(const LLPointer<LLFontFreetype>& fallback_font,
                                     const char_functor_t& functor)
{
    mFallbackFonts.emplace_back(fallback_font, functor);
}

F32 LLFontFreetype::getLineHeight() const
{
    return mLineHeight;
}

F32 LLFontFreetype::getAscenderHeight() const
{
    return mAscender;
}

F32 LLFontFreetype::getDescenderHeight() const
{
    return mDescender;
}

F32 LLFontFreetype::getXAdvance(llwchar wch) const
{
    if (mFTFace == nullptr)
        return 0.0;

    // Return existing info only if it is current
    LLFontGlyphInfo* gi = getGlyphInfo(wch, EFontGlyphType::Unspecified);
    if (gi)
    {
        return gi->mXAdvance;
    }
    else
    {
        char_glyph_info_map_t::iterator found_it = mCharGlyphInfoMap.find((llwchar)0);
        if (found_it != mCharGlyphInfoMap.end())
        {
            return found_it->second->mXAdvance;
        }
    }

    // Last ditch fallback - no glyphs defined at all.
    return (F32)mFontBitmapCachep->getMaxCharWidth();
}

F32 LLFontFreetype::getXAdvance(const LLFontGlyphInfo* glyph) const
{
    if (mFTFace == nullptr)
        return 0.0;

    return glyph->mXAdvance;
}

F32 LLFontFreetype::getXKerning(llwchar char_left, llwchar char_right) const
{
    if (mFTFace == nullptr)
        return 0.0;

    //llassert(!mIsFallback);
    LLFontGlyphInfo* left_glyph_info = getGlyphInfo(char_left, EFontGlyphType::Unspecified);;
    U32 left_glyph = left_glyph_info ? left_glyph_info->mGlyphIndex : 0;
    // Kern this puppy.
    LLFontGlyphInfo* right_glyph_info = getGlyphInfo(char_right, EFontGlyphType::Unspecified);
    U32 right_glyph = right_glyph_info ? right_glyph_info->mGlyphIndex : 0;

    FT_Vector  delta;

    llverify(!FT_Get_Kerning(mFTFace, left_glyph, right_glyph, ft_kerning_unfitted, &delta));

    return delta.x*(1.f/64.f);
}

F32 LLFontFreetype::getXKerning(const LLFontGlyphInfo* left_glyph_info, const LLFontGlyphInfo* right_glyph_info) const
{
    if (mFTFace == nullptr)
        return 0.0;

    U32 left_glyph = left_glyph_info ? left_glyph_info->mGlyphIndex : 0;
    U32 right_glyph = right_glyph_info ? right_glyph_info->mGlyphIndex : 0;

    FT_Vector  delta;

    llverify(!FT_Get_Kerning(mFTFace, left_glyph, right_glyph, ft_kerning_unfitted, &delta));

    return delta.x*(1.f/64.f);
}

bool LLFontFreetype::hasGlyph(llwchar wch) const
{
    llassert(!mIsFallback);
    return(mCharGlyphInfoMap.find(wch) != mCharGlyphInfoMap.end());
}

LLFontGlyphInfo* LLFontFreetype::addGlyph(llwchar wch, EFontGlyphType glyph_type) const
{
    if (!mFTFace)
    {
        return nullptr;
    }

    llassert(!mIsFallback);
    llassert(glyph_type < EFontGlyphType::Count);
    //LL_DEBUGS() << "Adding new glyph for " << wch << " to font" << LL_ENDL;

    // Initialize char to glyph map
    FT_UInt glyph_index = FT_Get_Char_Index(mFTFace, wch);
    if (glyph_index == 0)
    {
        // No corresponding glyph in this font: look for a glyph in fallback
        // fonts.
        size_t count = mFallbackFonts.size();
        if (LLStringOps::isEmoji(wch))
        {
            // This is a "genuine" emoji (in the range 0x1f000-0x20000): print
            // it using the emoji font(s) if possible. HB
            for (size_t i = 0; i < count; ++i)
            {
                const fallback_font_t& pair = mFallbackFonts[i];
                if (!pair.second || !pair.second(wch))
                {
                    // If this font does not have a functor, or the character
                    // does not pass the functor, reject it. Note: we keep the
                    // functor test (despite the fact we already tested for
                    // LLStringOps::isEmoji(wch) above), in case we would use
                    // different, more restrictive or partionned functors in
                    // the future with several different emoji fonts. HB
                    continue;
                }
                glyph_index = FT_Get_Char_Index(pair.first->mFTFace, wch);
            if (glyph_index)
            {
                    return addGlyphFromFont(pair.first, wch, glyph_index,
                                            glyph_type);
                }
            }
        }
        // Then try and find a monochrome fallback font that could print this
        // glyph: such fonts do *not* have a functor. We give priority to
        // monochrome fonts for non-genuine emojis so that UI elements which
        // used to render with them before the emojis font introduction (e.g.
        // check marks in menus, or LSL dialogs text and buttons) do render the
        // same way as they always did. HB
        std::vector<size_t> emoji_fonts_idx;
        for (size_t i = 0; i < count; ++i)
        {
            const fallback_font_t& pair = mFallbackFonts[i];
            if (pair.second)
            {
                // If this font got a functor, remember the index for later and
                // try the next fallback font. HB
                emoji_fonts_idx.push_back(i);
                continue;
            }
            glyph_index = FT_Get_Char_Index(pair.first->mFTFace, wch);
            if (glyph_index)
            {
                return addGlyphFromFont(pair.first, wch, glyph_index,
                                        glyph_type);
            }
        }
        // Everything failed so far: this character is not a genuine emoji,
        // neither a special character known from our monochrome fallback
        // fonts: make a last try, using the emoji font(s), but ignoring the
        // functor to render using whatever (colorful) glyph that might be
        // available in such fonts for this character. HB
        for (size_t j = 0, count2 = emoji_fonts_idx.size(); j < count2; ++j)
        {
            const fallback_font_t& pair = mFallbackFonts[emoji_fonts_idx[j]];
            glyph_index = FT_Get_Char_Index(pair.first->mFTFace, wch);
            if (glyph_index)
            {
                return addGlyphFromFont(pair.first, wch, glyph_index,
                                        glyph_type);
            }
        }
    }

    auto range_it = mCharGlyphInfoMap.equal_range(wch);
    char_glyph_info_map_t::iterator iter =
        std::find_if(range_it.first, range_it.second,
                     [&glyph_type](const char_glyph_info_map_t::value_type& entry)
                     {
                        return entry.second->mGlyphType == glyph_type;
                     });
    if (iter == range_it.second)
    {
        return addGlyphFromFont(this, wch, glyph_index, glyph_type);
    }
    return nullptr;
}

LLFontGlyphInfo* LLFontFreetype::addGlyphFromFont(const LLFontFreetype *fontp, llwchar wch, U32 glyph_index, EFontGlyphType requested_glyph_type) const
{
    LL_PROFILE_ZONE_SCOPED;
    if (mFTFace == nullptr)
        return nullptr;

    llassert(!mIsFallback);
    fontp->renderGlyph(requested_glyph_type, glyph_index);

    EFontGlyphType bitmap_glyph_type = EFontGlyphType::Unspecified;
    switch (fontp->mFTFace->glyph->bitmap.pixel_mode)
    {
        case FT_PIXEL_MODE_MONO:
        case FT_PIXEL_MODE_GRAY:
            bitmap_glyph_type = EFontGlyphType::Grayscale;
            break;
        case FT_PIXEL_MODE_BGRA:
            bitmap_glyph_type = EFontGlyphType::Color;
            break;
        default:
            llassert_always(true);
            break;
    }
    S32 width = fontp->mFTFace->glyph->bitmap.width;
    S32 height = fontp->mFTFace->glyph->bitmap.rows;

    S32 pos_x, pos_y;
    U32 bitmap_num;
    mFontBitmapCachep->nextOpenPos(width, pos_x, pos_y, bitmap_glyph_type, bitmap_num);
    mAddGlyphCount++;

    LLFontGlyphInfo* gi = new LLFontGlyphInfo(glyph_index, requested_glyph_type);
    gi->mXBitmapOffset = pos_x;
    gi->mYBitmapOffset = pos_y;
    gi->mBitmapEntry = std::make_pair(bitmap_glyph_type, bitmap_num);
    gi->mWidth = width;
    gi->mHeight = height;
    gi->mXBearing = fontp->mFTFace->glyph->bitmap_left;
    gi->mYBearing = fontp->mFTFace->glyph->bitmap_top;
    // Convert these from 26.6 units to float pixels.
    gi->mXAdvance = fontp->mFTFace->glyph->advance.x / 64.f;
    gi->mYAdvance = fontp->mFTFace->glyph->advance.y / 64.f;

    insertGlyphInfo(wch, gi);

    if (requested_glyph_type != bitmap_glyph_type)
    {
        LLFontGlyphInfo* gi_temp = new LLFontGlyphInfo(*gi);
        gi_temp->mGlyphType = bitmap_glyph_type;
        insertGlyphInfo(wch, gi_temp);
    }

    if (fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO
        || fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
    {
        U8 *buffer_data = fontp->mFTFace->glyph->bitmap.buffer;
        S32 buffer_row_stride = fontp->mFTFace->glyph->bitmap.pitch;
        U8 *tmp_graydata = nullptr;

        if (fontp->mFTFace->glyph->bitmap.pixel_mode
            == FT_PIXEL_MODE_MONO)
        {
            // need to expand 1-bit bitmap to 8-bit graymap.
            tmp_graydata = new U8[width * height];
            S32 xpos, ypos;
            for (ypos = 0; ypos < height; ++ypos)
            {
                S32 bm_row_offset = buffer_row_stride * ypos;
                for (xpos = 0; xpos < width; ++xpos)
                {
                    U32 bm_col_offsetbyte = xpos / 8;
                    U32 bm_col_offsetbit = 7 - (xpos % 8);
                    U32 bit =
                    !!(buffer_data[bm_row_offset
                               + bm_col_offsetbyte
                       ] & (1 << bm_col_offsetbit) );
                    tmp_graydata[width*ypos + xpos] =
                        255 * bit;
                }
            }
            // use newly-built graymap.
            buffer_data = tmp_graydata;
            buffer_row_stride = width;
        }

        setSubImageLuminanceAlpha(pos_x,
                                    pos_y,
                                    bitmap_num,
                                    width,
                                    height,
                                    buffer_data,
                                    buffer_row_stride);

        if (tmp_graydata)
            delete[] tmp_graydata;
    }
    else if (fontp->mFTFace->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA)
    {
        setSubImageBGRA(pos_x,
                        pos_y,
                        bitmap_num,
                        fontp->mFTFace->glyph->bitmap.width,
                        fontp->mFTFace->glyph->bitmap.rows,
                        fontp->mFTFace->glyph->bitmap.buffer,
                        llabs(fontp->mFTFace->glyph->bitmap.pitch));
    } else {
        llassert(false);
    }

    LLImageGL *image_gl = mFontBitmapCachep->getImageGL(bitmap_glyph_type, bitmap_num);
    LLImageRaw *image_raw = mFontBitmapCachep->getImageRaw(bitmap_glyph_type, bitmap_num);
    image_gl->setSubImage(image_raw, 0, 0, image_gl->getWidth(), image_gl->getHeight());

    return gi;
}

LLFontGlyphInfo* LLFontFreetype::getGlyphInfo(llwchar wch, EFontGlyphType glyph_type) const
{
    std::pair<char_glyph_info_map_t::iterator, char_glyph_info_map_t::iterator> range_it = mCharGlyphInfoMap.equal_range(wch);

    char_glyph_info_map_t::iterator iter = (EFontGlyphType::Unspecified != glyph_type)
        ? std::find_if(range_it.first, range_it.second, [&glyph_type](const char_glyph_info_map_t::value_type& entry) { return entry.second->mGlyphType == glyph_type; })
        : range_it.first;
    if (iter != range_it.second)
    {
        return iter->second;
    }
    else
    {
        // this glyph doesn't yet exist, so render it and return the result
        return addGlyph(wch, (EFontGlyphType::Unspecified != glyph_type) ? glyph_type : EFontGlyphType::Grayscale);
    }
}

void LLFontFreetype::insertGlyphInfo(llwchar wch, LLFontGlyphInfo* gi) const
{
    llassert(gi->mGlyphType < EFontGlyphType::Count);
    std::pair<char_glyph_info_map_t::iterator, char_glyph_info_map_t::iterator> range_it = mCharGlyphInfoMap.equal_range(wch);

    char_glyph_info_map_t::iterator iter =
        std::find_if(range_it.first, range_it.second, [&gi](const char_glyph_info_map_t::value_type& entry) { return entry.second->mGlyphType == gi->mGlyphType; });
    if (iter != range_it.second)
    {
        delete iter->second;
        iter->second = gi;
    }
    else
    {
        mCharGlyphInfoMap.insert(std::make_pair(wch, gi));
    }
}

void LLFontFreetype::renderGlyph(EFontGlyphType bitmap_type, U32 glyph_index) const
{
    if (mFTFace == nullptr)
        return;

    FT_Int32 load_flags = FT_LOAD_FORCE_AUTOHINT;
    if (EFontGlyphType::Color == bitmap_type)
    {
        // We may not actually get a color render so our caller should always examine mFTFace->glyph->bitmap.pixel_mode
        load_flags |= FT_LOAD_COLOR;
    }

    FT_Error error = FT_Load_Glyph(mFTFace, glyph_index, load_flags);
    if (FT_Err_Ok != error)
    {
        std::string message = llformat(
            "Error %d (%s) loading glyph %u: bitmap_type=%u, load_flags=%d",
            error, FT_Error_String(error), glyph_index, bitmap_type, load_flags);
        LL_WARNS_ONCE() << message << LL_ENDL;
        error = FT_Load_Glyph(mFTFace, glyph_index, load_flags ^ FT_LOAD_COLOR);
        llassert_always_msg(FT_Err_Ok == error, message.c_str());
    }

    llassert_always(! FT_Render_Glyph(mFTFace->glyph, gFontRenderMode) );

    mRenderGlyphCount++;
}

void LLFontFreetype::reset(F32 vert_dpi, F32 horz_dpi)
{
    resetBitmapCache();
    loadFace(mName, mPointSize, vert_dpi ,horz_dpi, mIsFallback, 0);
    if (!mIsFallback)
    {
        // This is the head of the list - need to rebuild ourself and all fallbacks.
        if (mFallbackFonts.empty())
        {
            LL_WARNS() << "LLFontGL::reset(), no fallback fonts present" << LL_ENDL;
        }
        else
        {
            for (fallback_font_vector_t::iterator it = mFallbackFonts.begin(); it != mFallbackFonts.end(); ++it)
            {
                it->first->reset(vert_dpi, horz_dpi);
            }
        }
    }
}

void LLFontFreetype::resetBitmapCache()
{
    for (char_glyph_info_map_t::iterator it = mCharGlyphInfoMap.begin(), end_it = mCharGlyphInfoMap.end();
        it != end_it;
        ++it)
    {
        delete it->second;
    }
    mCharGlyphInfoMap.clear();
    mFontBitmapCachep->reset();

    // Adding default glyph is skipped for fallback fonts here as well as in loadFace().
    // This if was added as fix for EXT-4971.
    if(!mIsFallback)
    {
        // Add the empty glyph
        addGlyphFromFont(this, 0, 0, EFontGlyphType::Grayscale);
    }
}

void LLFontFreetype::destroyGL()
{
    mFontBitmapCachep->destroyGL();
}

const std::string &LLFontFreetype::getName() const
{
    return mName;
}

static void dumpFontBitmap(const LLImageRaw* image_raw, const std::string& file_name)
{
    LLPointer<LLImagePNG> tmpImage = new LLImagePNG();
    if ( (tmpImage->encode(image_raw, 0.0f)) && (tmpImage->save(gDirUtilp->getExpandedFilename(LL_PATH_LOGS, file_name))) )
    {
        LL_INFOS("Font") << "Successfully saved " << file_name << LL_ENDL;
    }
    else
    {
        LL_WARNS("Font") << "Failed to save " << file_name << LL_ENDL;
    }
}

void LLFontFreetype::dumpFontBitmaps() const
{
    // Dump all the regular bitmaps (if any)
    for (int idx = 0, cnt = mFontBitmapCachep->getNumBitmaps(EFontGlyphType::Grayscale); idx < cnt; idx++)
    {
        dumpFontBitmap(mFontBitmapCachep->getImageRaw(EFontGlyphType::Grayscale, idx), llformat("%s_%d_%d_%d.png", mFTFace->family_name, (int)(mPointSize * 10), mStyle, idx));
    }

    // Dump all the color bitmaps (if any)
    for (int idx = 0, cnt = mFontBitmapCachep->getNumBitmaps(EFontGlyphType::Color); idx < cnt; idx++)
    {
        dumpFontBitmap(mFontBitmapCachep->getImageRaw(EFontGlyphType::Color, idx), llformat("%s_%d_%d_%d_clr.png", mFTFace->family_name, (int)(mPointSize * 10), mStyle, idx));
    }
}

const LLFontBitmapCache* LLFontFreetype::getFontBitmapCache() const
{
    return mFontBitmapCachep;
}

void LLFontFreetype::setStyle(U8 style)
{
    mStyle = style;
}

U8 LLFontFreetype::getStyle() const
{
    return mStyle;
}

bool LLFontFreetype::setSubImageBGRA(U32 x, U32 y, U32 bitmap_num, U16 width, U16 height, const U8* data, U32 stride) const
{
    LLImageRaw* image_raw = mFontBitmapCachep->getImageRaw(EFontGlyphType::Color, bitmap_num);
    llassert(!mIsFallback);
    llassert(image_raw && (image_raw->getComponents() == 4));

    // NOTE: inspired by LLImageRaw::setSubImage()
    U32* image_data = (U32*)image_raw->getData();
    if (!image_data)
    {
        return false;
    }

    for (U32 idxRow = 0; idxRow < height; idxRow++)
    {
        const U32 nSrcRow = height - 1 - idxRow;
        const U32 nSrcOffset = nSrcRow * width * image_raw->getComponents();
        const U32 nDstOffset = (y + idxRow) * image_raw->getWidth() + x;

        for (U32 idxCol = 0; idxCol < width; idxCol++)
        {
            U32 nTemp = nSrcOffset + idxCol * 4;
            image_data[nDstOffset + idxCol] = data[nTemp + 3] << 24 | data[nTemp] << 16 | data[nTemp + 1] << 8 | data[nTemp + 2];
        }
    }

    return true;
}

void LLFontFreetype::setSubImageLuminanceAlpha(U32 x, U32 y, U32 bitmap_num, U32 width, U32 height, U8 *data, S32 stride) const
{
    LLImageRaw *image_raw = mFontBitmapCachep->getImageRaw(EFontGlyphType::Grayscale, bitmap_num);
    LLImageDataLock lock(image_raw);

    llassert(!mIsFallback);
    llassert(image_raw && (image_raw->getComponents() == 2));

    U8 *target = image_raw->getData();
    llassert(target);

    if (!data || !target)
    {
        return;
    }

    if (0 == stride)
        stride = width;

    U32 i, j;
    U32 to_offset;
    U32 from_offset;
    U32 target_width = image_raw->getWidth();
    for (i = 0; i < height; i++)
    {
        to_offset = (y + i)*target_width + x;
        from_offset = (height - 1 - i)*stride;
        for (j = 0; j < width; j++)
        {
            *(target + to_offset*2 + 1) = *(data + from_offset);
            to_offset++;
            from_offset++;
        }
    }
}


namespace ll
{
    namespace fonts
    {
        class LoadedFont
        {
            public:
            LoadedFont( std::string aName , std::string const &aAddress, std::size_t aSize )
            : mAddress( aAddress )
            {
                mName = aName;
                mSize = aSize;
                mRefs = 1;
            }
            std::string mName;
            std::string mAddress;
            std::size_t mSize;
            U32  mRefs;
        };
    }
}

U8 const* LLFontManager::loadFont( std::string const &aFilename, long &a_Size)
{
    a_Size = 0;
    std::map< std::string, std::shared_ptr<ll::fonts::LoadedFont> >::iterator itr = m_LoadedFonts.find( aFilename );
    if( itr != m_LoadedFonts.end() )
    {
        ++itr->second->mRefs;
        // A possible overflow cannot happen here, as it is asserted that the size is less than std::numeric_limits<long>::max() a few lines below.
        a_Size = static_cast<long>(itr->second->mSize);
        return reinterpret_cast<U8 const*>(itr->second->mAddress.c_str());
    }

    auto strContent = LLFile::getContents(aFilename);

    if( strContent.empty() )
        return nullptr;

    // For fontconfig a type of long is required, std::string::size() returns size_t. I think it is safe to limit this to 2GiB and not support fonts that huge (can that even be a thing?)
    llassert_always( strContent.size() < std::numeric_limits<long>::max() );

    a_Size = static_cast<long>(strContent.size());

    auto pCache = std::make_shared<ll::fonts::LoadedFont>( aFilename,  strContent, a_Size );
    itr = m_LoadedFonts.insert( std::make_pair( aFilename, pCache ) ).first;

    return reinterpret_cast<U8 const*>(itr->second->mAddress.c_str());
}

void LLFontManager::unloadAllFonts()
{
    m_LoadedFonts.clear();
}
