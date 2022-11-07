/** 
 * @file llviewerlayer.h
 * @brief LLViewerLayer class header file
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

#ifndef LL_LLVIEWERLAYER_H
#define LL_LLVIEWERLAYER_H

// Viewer-side representation of a layer...

class LLViewerLayer
{
public:
    LLViewerLayer(const S32 width, const F32 scale = 1.f);
    virtual ~LLViewerLayer();

    F32 getValueScaled(const F32 x, const F32 y) const;
protected:
    F32 getValue(const S32 x, const S32 y) const;
protected:
    S32 mWidth;
    F32 mScale;
    F32 mScaleInv;
    F32 *mDatap;
};

#endif // LL_LLVIEWERLAYER_H
