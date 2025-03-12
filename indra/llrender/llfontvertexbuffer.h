/**
 * @file llfontgl.h
 * @author Andrii Kleshchev
 * @brief Buffer storage for font rendering.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLFONTVERTEXBUFFER_H
#define LL_LLFONTVERTEXBUFFER_H

#include "llfontgl.h"

class LLVertexBufferData;

class LLFontVertexBuffer
{
public:
    LLFontVertexBuffer();
    ~LLFontVertexBuffer();

    void reset();

    S32 render(const LLFontGL* fontp,
        const LLWString& text,
        S32 begin_offset,
        LLRect rect,
        const LLColor4& color,
        LLFontGL::HAlign halign = LLFontGL::LEFT, LLFontGL::VAlign valign = LLFontGL::BASELINE,
        U8 style = LLFontGL::NORMAL,
        LLFontGL::ShadowType shadow = LLFontGL::NO_SHADOW,
        S32 max_chars = S32_MAX, S32 max_pixels = S32_MAX,
        F32* right_x = NULL,
        bool use_ellipses = false,
        bool use_color = true);

    S32 render(const LLFontGL* fontp,
        const LLWString& text,
        S32 begin_offset,
        LLRectf rect,
        const LLColor4& color,
        LLFontGL::HAlign halign = LLFontGL::LEFT, LLFontGL::VAlign valign = LLFontGL::BASELINE,
        U8 style = LLFontGL::NORMAL,
        LLFontGL::ShadowType shadow = LLFontGL::NO_SHADOW,
        S32 max_chars = S32_MAX,
        F32* right_x = NULL,
        bool use_ellipses = false,
        bool use_color = true);

    S32 render(const LLFontGL* fontp,
        const LLWString& text,
        S32 begin_offset,
        F32 x, F32 y,
        const LLColor4& color,
        LLFontGL::HAlign halign = LLFontGL::LEFT, LLFontGL::VAlign valign = LLFontGL::BASELINE,
        U8 style = LLFontGL::NORMAL,
        LLFontGL::ShadowType shadow = LLFontGL::NO_SHADOW,
        S32 max_chars = S32_MAX, S32 max_pixels = S32_MAX,
        F32* right_x = NULL,
        bool use_ellipses = false,
        bool use_color = true);

    static void enableBufferCollection(bool enable) { sEnableBufferCollection = enable; }
private:

    void genBuffers(const LLFontGL* fontp,
         const LLWString& text,
         S32 begin_offset,
         F32 x, F32 y,
         const LLColor4& color,
         LLFontGL::HAlign halign, LLFontGL::VAlign valign,
         U8 style,
        LLFontGL::ShadowType shadow,
         S32 max_chars, S32 max_pixels,
         F32* right_x,
         bool use_ellipses,
         bool use_color);

    void renderBuffers();

    std::list<LLVertexBufferData> mBufferList;
    S32 mChars = 0;
    const LLFontGL *mLastFont = nullptr;
    S32 mLastOffset = 0;
    S32 mLastMaxChars = 0;
    S32 mLastMaxPixels = 0;
    F32 mLastX = 0.f;
    F32 mLastY = 0.f;
    LLColor4 mLastColor;
    LLFontGL::HAlign mLastHalign = LLFontGL::LEFT;
    LLFontGL::VAlign mLastValign = LLFontGL::BASELINE;
    U8 mLastStyle = LLFontGL::NORMAL;
    LLFontGL::ShadowType mLastShadow = LLFontGL::NO_SHADOW;
    F32 mLastRightX = 0.f;

    // LLFontGL's statics
    F32 mLastScaleX = 1.f;
    F32 mLastScaleY = 1.f;
    F32 mLastVertDPI = 0.f;
    F32 mLastHorizDPI = 0.f;
    S32 mLastResGeneration = 0;
    LLCoordGL mLastOrigin;

    // Adding new characters to bitmap cache can alter value from getBitmapWidth();
    // which alters whole string. So rerender when new characters were added to cache.
    S32 mLastFontCacheGen = 0;

    static bool sEnableBufferCollection;
};

#endif
