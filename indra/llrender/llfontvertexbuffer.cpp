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


LLFontVertexBuffer::LLFontVertexBuffer(bool track_changes)
: mTrackStringChanges(track_changes)
{
}

LLFontVertexBuffer::~LLFontVertexBuffer()
{
    reset();
}

void LLFontVertexBuffer::reset()
{
    mBufferList.clear();
}

S32 LLFontVertexBuffer::render(
    const LLFontGL* fontp,
    const LLWString& text,
    S32 begin_offset,
    F32 x, F32 y,
    const LLColor4& color,
    LLFontGL::HAlign halign, LLFontGL::VAlign valign,
    U8 style, LLFontGL::ShadowType shadow,
    S32 max_chars , S32 max_pixels,
    F32* right_x,
    bool use_ellipses,
    bool use_color )
{
    if (mBufferList.empty())
    {
        genBuffers(fontp, text, begin_offset, x, y, color, halign, valign,
            style, shadow, max_chars, max_pixels, right_x, use_ellipses, use_color);
    }
    else if (mLastX != x || mLastY != y || mLastColor != color) // always track position and alphs
    {
        genBuffers(fontp, text, begin_offset, x, y, color, halign, valign,
            style, shadow, max_chars, max_pixels, right_x, use_ellipses, use_color);
    }
    else if (true //mTrackStringChanges
             && (mLastOffset != begin_offset
                 || mLastMaxChars != max_chars
                 || mLastMaxPixels != max_pixels
                 || mLastStringHash != sStringHasher._Do_hash(text))) // todo, track all parameters?
    {
        genBuffers(fontp, text, begin_offset, x, y, color, halign, valign,
            style, shadow, max_chars, max_pixels, right_x, use_ellipses, use_color);
    }
    else
    {

        gGL.flush(); // deliberately empty pending verts
        gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
        gGL.pushUIMatrix();

        gGL.loadUIIdentity();

        // Depth translation, so that floating text appears 'in-world'
        // and is correctly occluded.
        gGL.translatef(0.f, 0.f, LLFontGL::sCurDepth);

        gGL.setSceneBlendType(LLRender::BT_ALPHA);

        for (auto &buf_data : mBufferList)
        {
            if (buf_data.mImage)
            {
                gGL.getTexUnit(0)->bind(buf_data.mImage);
            }
            else
            {
                gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            }
            buf_data.mBuffer->setBuffer();

            if (LLRender::sGLCoreProfile && buf_data.mMode == LLRender::QUADS)
            {
                buf_data.mBuffer->drawArrays(LLRender::TRIANGLES, 0, buf_data.mCount);
            }
            else
            {
                buf_data.mBuffer->drawArrays(buf_data.mMode, 0, buf_data.mCount);
            }
        }
        if (right_x)
        {
            *right_x = mLastRightX;
        }

        gGL.popUIMatrix();
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
    mBufferList.clear();
    mChars = fontp->render(text, begin_offset, x, y, color, halign, valign,
        style, shadow, max_chars, max_pixels, right_x, use_ellipses, use_color, &mBufferList);

    mLastOffset = begin_offset;
    mLastMaxChars = max_chars;
    mLastMaxPixels = max_pixels;
    mLastStringHash = sStringHasher._Do_hash(text);
    mLastX = x;
    mLastY = y;
    mLastColor = color;

    if (right_x)
    {
        mLastRightX = *right_x;
    }
}

