/** 
 * @file llpanelpulldown.h
 * @brief A panel that serves as a basis for multiple toolbar pulldown panels
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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

#ifndef LL_LLPANELPULLDOWN_H
#define LL_LLPANELPULLDOWN_H

#include "linden_common.h"

#include "llpanel.h"

class LLFrameTimer;

class LLPanelPulldown : public LLPanel
{
public:
    LLPanelPulldown();
    /*virtual*/ void onMouseEnter(S32 x, S32 y, MASK mask);
    /*virtual*/ void onMouseLeave(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL handleDoubleClick(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
    /*virtual*/ void onTopLost();
    /*virtual*/ void onVisibilityChange(BOOL new_visibility);

    /*virtual*/ void draw();

protected:
    LLFrameTimer mHoverTimer;
};

#endif // LL_LLPANELPULLDOWN_H
