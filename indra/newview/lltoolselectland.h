/** 
 * @file lltoolselectland.h
 * @brief LLToolSelectLand class header file
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

#ifndef LL_LLTOOLSELECTLAND_H
#define LL_LLTOOLSELECTLAND_H

#include "lltool.h"
#include "v3dmath.h"

class LLParcelSelection;

class LLToolSelectLand
:   public LLTool, public LLSingleton<LLToolSelectLand>
{
    LLSINGLETON(LLToolSelectLand);
    virtual ~LLToolSelectLand();

public:
    /*virtual*/ BOOL        handleMouseDown(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL        handleDoubleClick(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL        handleMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL        handleHover(S32 x, S32 y, MASK mask);
    /*virtual*/ void        render();               // draw the select rectangle
    /*virtual*/ BOOL        isAlwaysRendered()      { return TRUE; }

    /*virtual*/ void        handleSelect();
    /*virtual*/ void        handleDeselect();

protected:
    BOOL            outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y);
    void            roundXY(LLVector3d& vec);

protected:
    LLVector3d      mDragStartGlobal;       // global coords
    LLVector3d      mDragEndGlobal;         // global coords
    BOOL            mDragEndValid;          // is drag end a valid point in the world?

    S32             mDragStartX;            // screen coords, from left
    S32             mDragStartY;            // screen coords, from bottom

    S32             mDragEndX;
    S32             mDragEndY;

    BOOL            mMouseOutsideSlop;      // has mouse ever gone outside slop region?

    LLVector3d      mWestSouthBottom;       // global coords, from drag
    LLVector3d      mEastNorthTop;          // global coords, from drag

    LLSafeHandle<LLParcelSelection> mSelection;     // hold on to a parcel selection
};


#endif
