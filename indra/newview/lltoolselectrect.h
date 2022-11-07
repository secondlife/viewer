/** 
 * @file lltoolselectrect.h
 * @brief A tool to select multiple objects with a screen-space rectangle.
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

#ifndef LL_LLTOOLSELECTRECT_H
#define LL_LLTOOLSELECTRECT_H

#include "lltool.h"
#include "lltoolselect.h"

class LLToolSelectRect
:   public LLToolSelect
{
public:
    LLToolSelectRect( LLToolComposite* composite );

    virtual BOOL    handleMouseDown(S32 x, S32 y, MASK mask);
    virtual BOOL    handleMouseUp(S32 x, S32 y, MASK mask);
    virtual BOOL    handleHover(S32 x, S32 y, MASK mask);
    virtual void    draw();                         // draw the select rectangle

    void handlePick(const LLPickInfo& pick);

protected:
    void            handleRectangleSelection(S32 x, S32 y, MASK mask);  // true if you selected one
    BOOL            outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y);

protected:
    S32             mDragStartX;                    // screen coords, from left
    S32             mDragStartY;                    // screen coords, from bottom

    S32             mDragEndX;
    S32             mDragEndY;

    S32             mDragLastWidth;
    S32             mDragLastHeight;

    BOOL            mMouseOutsideSlop;      // has mouse ever gone outside slop region?
};


#endif
