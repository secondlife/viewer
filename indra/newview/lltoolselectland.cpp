/** 
 * @file lltoolselectland.cpp
 * @brief LLToolSelectLand class implementation
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

#include "llviewerprecompiledheaders.h"

#include "lltoolselectland.h"

// indra includes
#include "llparcel.h"

// Viewer includes
#include "llviewercontrol.h"
#include "llfloatertools.h"
#include "llselectmgr.h"
#include "llstatusbar.h"
#include "llviewerparcelmgr.h"
#include "llviewerwindow.h"

//
// Member functions
//

LLToolSelectLand::LLToolSelectLand( )
:   LLTool( std::string("Parcel") ),
    mDragStartGlobal(),
    mDragEndGlobal(),
    mDragEndValid(FALSE),
    mDragStartX(0),
    mDragStartY(0),
    mDragEndX(0),
    mDragEndY(0),
    mMouseOutsideSlop(FALSE),
    mWestSouthBottom(),
    mEastNorthTop()
{ }

LLToolSelectLand::~LLToolSelectLand()
{
}


BOOL LLToolSelectLand::handleMouseDown(S32 x, S32 y, MASK mask)
{
    BOOL hit_land = gViewerWindow->mousePointOnLandGlobal(x, y, &mDragStartGlobal);
    if (hit_land)
    {
        setMouseCapture( TRUE );

        mDragStartX = x;
        mDragStartY = y;
        mDragEndX = x;
        mDragEndY = y;

        mDragEndValid       = TRUE;
        mDragEndGlobal      = mDragStartGlobal;

        sanitize_corners(mDragStartGlobal, mDragEndGlobal, mWestSouthBottom, mEastNorthTop);

        mWestSouthBottom -= LLVector3d( PARCEL_GRID_STEP_METERS/2, PARCEL_GRID_STEP_METERS/2, 0 );
        mEastNorthTop += LLVector3d( PARCEL_GRID_STEP_METERS/2, PARCEL_GRID_STEP_METERS/2, 0 );

        roundXY(mWestSouthBottom);
        roundXY(mEastNorthTop);

        mMouseOutsideSlop = TRUE; //FALSE;

        LLViewerParcelMgr::getInstance()->deselectLand();
    }

    return hit_land;
}


BOOL LLToolSelectLand::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    LLVector3d pos_global;
    BOOL hit_land = gViewerWindow->mousePointOnLandGlobal(x, y, &pos_global);
    if (hit_land)
    {
        // Auto-select this parcel
        LLViewerParcelMgr::getInstance()->selectParcelAt( pos_global );
        return TRUE;
    }
    return FALSE;
}


BOOL LLToolSelectLand::handleMouseUp(S32 x, S32 y, MASK mask)
{
    if( hasMouseCapture() )
    {
        setMouseCapture( FALSE );

        if (mMouseOutsideSlop && mDragEndValid)
        {
            // Take the drag start and end locations, then map the southwest
            // point down to the next grid location, and the northeast point up
            // to the next grid location.

            sanitize_corners(mDragStartGlobal, mDragEndGlobal, mWestSouthBottom, mEastNorthTop);

            mWestSouthBottom -= LLVector3d( PARCEL_GRID_STEP_METERS/2, PARCEL_GRID_STEP_METERS/2, 0 );
            mEastNorthTop += LLVector3d( PARCEL_GRID_STEP_METERS/2, PARCEL_GRID_STEP_METERS/2, 0 );

            roundXY(mWestSouthBottom);
            roundXY(mEastNorthTop);

            // Don't auto-select entire parcel.
            mSelection = LLViewerParcelMgr::getInstance()->selectLand( mWestSouthBottom, mEastNorthTop, FALSE );
        }

        mMouseOutsideSlop = FALSE;
        mDragEndValid = FALSE;
        
        return TRUE;
    }
    return FALSE;
}


BOOL LLToolSelectLand::handleHover(S32 x, S32 y, MASK mask)
{
    if( hasMouseCapture() )
    {
        if (mMouseOutsideSlop || outsideSlop(x, y, mDragStartX, mDragStartY))
        {
            mMouseOutsideSlop = TRUE;

            // Must do this every frame, in case the camera moved or the land moved
            // since last frame.

            // If doesn't hit land, doesn't change old value
            LLVector3d land_global;
            BOOL hit_land = gViewerWindow->mousePointOnLandGlobal(x, y, &land_global);
            if (hit_land)
            {
                mDragEndValid = TRUE;
                mDragEndGlobal = land_global;

                sanitize_corners(mDragStartGlobal, mDragEndGlobal, mWestSouthBottom, mEastNorthTop);

                mWestSouthBottom -= LLVector3d( PARCEL_GRID_STEP_METERS/2, PARCEL_GRID_STEP_METERS/2, 0 );
                mEastNorthTop += LLVector3d( PARCEL_GRID_STEP_METERS/2, PARCEL_GRID_STEP_METERS/2, 0 );

                roundXY(mWestSouthBottom);
                roundXY(mEastNorthTop);

                LL_DEBUGS("UserInput") << "hover handled by LLToolSelectLand (active, land)" << LL_ENDL;
                gViewerWindow->setCursor(UI_CURSOR_ARROW);
            }
            else
            {
                mDragEndValid = FALSE;
                LL_DEBUGS("UserInput") << "hover handled by LLToolSelectLand (active, no land)" << LL_ENDL;
                gViewerWindow->setCursor(UI_CURSOR_NO);
            }

            mDragEndX = x;
            mDragEndY = y;
        }
        else
        {
            LL_DEBUGS("UserInput") << "hover handled by LLToolSelectLand (active, in slop)" << LL_ENDL;
            gViewerWindow->setCursor(UI_CURSOR_ARROW);
        }
    }
    else
    {
        LL_DEBUGS("UserInput") << "hover handled by LLToolSelectLand (inactive)" << LL_ENDL;        
        gViewerWindow->setCursor(UI_CURSOR_ARROW);
    }

    return TRUE;
}


void LLToolSelectLand::render()
{
    if( hasMouseCapture() && /*mMouseOutsideSlop &&*/ mDragEndValid)
    {
        LLViewerParcelMgr::getInstance()->renderRect( mWestSouthBottom, mEastNorthTop );
    }
}

void LLToolSelectLand::handleSelect()
{
    gFloaterTools->setStatusText("selectland");
}


void LLToolSelectLand::handleDeselect()
{
    mSelection = NULL;
}


void LLToolSelectLand::roundXY(LLVector3d &vec)
{
    vec.mdV[VX] = ll_round( vec.mdV[VX], (F64)PARCEL_GRID_STEP_METERS );
    vec.mdV[VY] = ll_round( vec.mdV[VY], (F64)PARCEL_GRID_STEP_METERS );
}


// true if x,y outside small box around start_x,start_y
BOOL LLToolSelectLand::outsideSlop(S32 x, S32 y, S32 start_x, S32 start_y)
{
    S32 dx = x - start_x;
    S32 dy = y - start_y;

    return (dx <= -2 || 2 <= dx || dy <= -2 || 2 <= dy);
}
