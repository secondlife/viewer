/**
 * @file llfontvertexbuffer.cpp
 * @brief Buffer storage for font rendering.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#include "llfontvertexbuffer.h"

#include "llvertexbuffer.h"


bool LLFontVertexBuffer::sEnableBufferCollection = true;

LLFontVertexBuffer::LLFontVertexBuffer()
{
}

LLFontVertexBuffer::~LLFontVertexBuffer()
{
    reset();
}

void LLFontVertexBuffer::reset()
{
    // Todo: some form of debug only frequecy check&assert to see if this is happening too often.
    // Regenerating this list is expensive
    mBufferList.clear();
}

S32 LLFontVertexBuffer::render(
    const LLFontGL* fontp,
    const LLWString& text,
    S32 begin_offset,
    LLRect rect,
    const LLColor4& color,
    LLFontGL::HAlign halign, LLFontGL::VAlign valign,
    U8 style,
    LLFontGL::ShadowType shadow,
    S32 max_chars, S32 max_pixels,
    F32* right_x,
    bool use_ellipses,
    bool use_color)
{
    LLRectf rect_float((F32)rect.mLeft, (F32)rect.mTop, (F32)rect.mRight, (F32)rect.mBottom);
    return render(fontp, text, begin_offset, rect_float, color, halign, valign, style, shadow, max_chars, right_x, use_ellipses, use_color);
}

S32 LLFontVertexBuffer::render(
    const LLFontGL* fontp,
    const LLWString& text,
    S32 begin_offset,
    LLRectf rect,
    const LLColor4& color,
    LLFontGL::HAlign halign, LLFontGL::VAlign valign,
    U8 style,
    LLFontGL::ShadowType shadow,
    S32 max_chars,
    F32* right_x,
    bool use_ellipses,
    bool use_color)
{
    F32 x = rect.mLeft;
    F32 y = 0.f;

    switch (valign)
    {
    case LLFontGL::TOP:
        y = rect.mTop;
        break;
    case LLFontGL::VCENTER:
        y = rect.getCenterY();
        break;
    case LLFontGL::BASELINE:
    case LLFontGL::BOTTOM:
        y = rect.mBottom;
        break;
    default:
        y = rect.mBottom;
        break;
    }
    return render(fontp, text, begin_offset, x, y, color, halign, valign, style, shadow, max_chars, (S32)rect.getWidth(), right_x, use_ellipses, use_color);
}

S32 LLFontVertexBuffer::render(
    const LLFontGL* fontp,
    const LLWString& text,
    S32 begin_offset,
    F32 x, F32 y,
    const LLColor4& color,
    LLFontGL::HAlign halign, LLFontGL::VAlign valign,
    U8 style,
    LLFontGL::ShadowType shadow,
    S32 max_chars , S32 max_pixels,
    F32* right_x,
    bool use_ellipses,
    bool use_color )
{
    if (!LLFontGL::sDisplayFont) //do not display texts
    {
        return static_cast<S32>(text.length());
    }
    if (!sEnableBufferCollection)
    {
        // For debug purposes and performance testing
        return fontp->render(text, begin_offset, x, y, color, halign, valign, style, shadow, max_chars, max_pixels, right_x, use_ellipses, use_color);
    }
    if (mBufferList.empty())
    {
        genBuffers(fontp, text, begin_offset, x, y, color, halign, valign,
            style, shadow, max_chars, max_pixels, right_x, use_ellipses, use_color);
    }
    else if (mLastX != x
             || mLastY != y
             || mLastFont != fontp
             || mLastColor != color // alphas change often
             || mLastHalign != halign
             || mLastValign != valign
             || mLastOffset != begin_offset
             || mLastMaxChars != max_chars
             || mLastMaxPixels != max_pixels
             || mLastStyle != style
             || mLastShadow != shadow // ex: buttons change shadow state
             || mLastScaleX != LLFontGL::sScaleX
             || mLastScaleY != LLFontGL::sScaleY
             || mLastVertDPI != LLFontGL::sVertDPI
             || mLastHorizDPI != LLFontGL::sHorizDPI
             || mLastOrigin != LLFontGL::sCurOrigin
             || mLastResGeneration != LLFontGL::sResolutionGeneration
             || mLastFontCacheGen != fontp->getCacheGeneration())
    {
        genBuffers(fontp, text, begin_offset, x, y, color, halign, valign,
            style, shadow, max_chars, max_pixels, right_x, use_ellipses, use_color);
    }
    else
    {
        renderBuffers();

        if (right_x)
        {
            *right_x = mLastRightX;
        }
    }
    return mChars;
}

void LLFontVertexBuffer::genBuffers(
    const LLFontGL* fontp,
    const LLWString& text,
    S32 begin_offset,
    F32 x, F32 y,
    const LLColor4& color,
    LLFontGL::HAlign halign, LLFontGL::VAlign valign,
    U8 style, LLFontGL::ShadowType shadow,
    S32 max_chars, S32 max_pixels,
    F32* right_x,
    bool use_ellipses,
    bool use_color)
{
    // todo: add a debug build assert if this triggers too often for to long?
    mBufferList.clear();
    // Save before rendreing, it can change mid-render,
    // so will need to rerender previous characters
    mLastFontCacheGen = fontp->getCacheGeneration();

    gGL.beginList(&mBufferList);
    mChars = fontp->render(text, begin_offset, x, y, color, halign, valign,
        style, shadow, max_chars, max_pixels, right_x, use_ellipses, use_color);
    gGL.endList();

    mLastFont = fontp;
    mLastOffset = begin_offset;
    mLastMaxChars = max_chars;
    mLastMaxPixels = max_pixels;
    mLastX = x;
    mLastY = y;
    mLastColor = color;
    mLastHalign = halign;
    mLastValign = valign;
    mLastStyle = style;
    mLastShadow = shadow;

    mLastScaleX = LLFontGL::sScaleX;
    mLastScaleY = LLFontGL::sScaleY;
    mLastVertDPI = LLFontGL::sVertDPI;
    mLastHorizDPI = LLFontGL::sHorizDPI;
    mLastOrigin = LLFontGL::sCurOrigin;
    mLastResGeneration = LLFontGL::sResolutionGeneration;

    if (right_x)
    {
        mLastRightX = *right_x;
    }
}

void LLFontVertexBuffer::renderBuffers()
{
    gGL.flush(); // deliberately empty pending verts
    gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
    gGL.pushUIMatrix();

    gGL.loadUIIdentity();

    // Depth translation, so that floating text appears 'in-world'
    // and is correctly occluded.
    gGL.translatef(0.f, 0.f, LLFontGL::sCurDepth);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    // Note: ellipses should technically be covered by push/load/translate of their own
    // but it's more complexity, values do not change, skipping doesn't appear to break
    // anything, so we can skip that until it proves to cause issues.
    for (LLVertexBufferData& buffer : mBufferList)
    {
        buffer.draw();
    }
    gGL.popUIMatrix();
}

