/** 
 * @file lltoolobjpicker.cpp
 * @brief LLToolObjPicker class implementation
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

// LLToolObjPicker is a transient tool, useful for a single object pick.

#include "llviewerprecompiledheaders.h"

#include "lltoolobjpicker.h"

#include "llagent.h"
#include "llselectmgr.h"
#include "llworld.h"
#include "llviewercontrol.h"
#include "llmenugl.h"
#include "lltoolmgr.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewermenu.h"
#include "llviewercamera.h"
#include "llviewerwindow.h"
#include "lldrawable.h"
#include "llrootview.h"


LLToolObjPicker::LLToolObjPicker()
:   LLTool( std::string("ObjPicker"), NULL ),
    mPicked( FALSE ),
    mHitObjectID( LLUUID::null ),
    mExitCallback( NULL ),
    mExitCallbackData( NULL )
{ }


// returns TRUE if an object was selected 
BOOL LLToolObjPicker::handleMouseDown(S32 x, S32 y, MASK mask)
{
    LLRootView* viewp = gViewerWindow->getRootView();
    BOOL handled = viewp->handleMouseDown(x, y, mask);

    mHitObjectID.setNull();

    if (! handled)
    {
        // didn't click in any UI object, so must have clicked in the world
        gViewerWindow->pickAsync(x, y, mask, pickCallback);
        handled = TRUE;
    }
    else
    {
        if (hasMouseCapture())
        {
            setMouseCapture(FALSE);
        }
        else
        {
            LL_WARNS() << "PickerTool doesn't have mouse capture on mouseDown" << LL_ENDL;  
        }
    }

    // Pass mousedown to base class
    LLTool::handleMouseDown(x, y, mask);

    return handled;
}

void LLToolObjPicker::pickCallback(const LLPickInfo& pick_info)
{
    LLToolObjPicker::getInstance()->mHitObjectID = pick_info.mObjectID;
    LLToolObjPicker::getInstance()->mPicked = pick_info.mObjectID.notNull();
}


BOOL LLToolObjPicker::handleMouseUp(S32 x, S32 y, MASK mask)
{
    LLView* viewp = gViewerWindow->getRootView();
    BOOL handled = viewp->handleHover(x, y, mask);
    if (handled)
    {
        // let UI handle this
    }

    LLTool::handleMouseUp(x, y, mask);
    if (hasMouseCapture())
    {
        setMouseCapture(FALSE);
    }
    else
    {
        LL_WARNS() << "PickerTool doesn't have mouse capture on mouseUp" << LL_ENDL;    
    }
    return handled;
}


BOOL LLToolObjPicker::handleHover(S32 x, S32 y, MASK mask)
{
    LLView *viewp = gViewerWindow->getRootView();
    BOOL handled = viewp->handleHover(x, y, mask);
    if (!handled) 
    {
        // Used to do pick on hover.  Now we just always display the cursor.
        ECursorType cursor = UI_CURSOR_ARROWLOCKED;

        cursor = UI_CURSOR_TOOLPICKOBJECT3;

        gViewerWindow->setCursor(cursor);
    }
    return handled;
}


void LLToolObjPicker::onMouseCaptureLost()
{
    if (mExitCallback)
    {
        mExitCallback(mExitCallbackData);

        mExitCallback = NULL;
        mExitCallbackData = NULL;
    }

    mPicked = FALSE;
    mHitObjectID.setNull();
}

// virtual
void LLToolObjPicker::setExitCallback(void (*callback)(void *), void *callback_data)
{
    mExitCallback = callback;
    mExitCallbackData = callback_data;
}

// virtual
void LLToolObjPicker::handleSelect()
{
    LLTool::handleSelect();
    setMouseCapture(TRUE);
}

// virtual
void LLToolObjPicker::handleDeselect()
{
    if (hasMouseCapture())
    {
        LLTool::handleDeselect();
        setMouseCapture(FALSE);
    }
}



