/** 
 * @file lldelayedgestureerror.cpp
 * @brief Delayed gesture error message -- try to wait until name has been retrieved
 * @author Dale Glass
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "lldelayedgestureerror.h"

#include <list>
#include "llnotificationsutil.h"
#include "llcallbacklist.h"
#include "llinventory.h"
#include "llviewerinventory.h"
#include "llinventorymodel.h"

const F32 MAX_NAME_WAIT_TIME = 5.0f;

LLDelayedGestureError::ErrorQueue LLDelayedGestureError::sQueue;

//static
void LLDelayedGestureError::gestureMissing(const LLUUID &id)
{
    LLErrorEntry ent("GestureMissing", id);
    if ( ! doDialog(ent) )
    {
        enqueue(ent);
    }
}

//static
void LLDelayedGestureError::gestureFailedToLoad(const LLUUID &id)
{
    LLErrorEntry ent("UnableToLoadGesture", id);

    if ( ! doDialog(ent) )
    {
        enqueue(ent);
    }
}

//static
void LLDelayedGestureError::enqueue(const LLErrorEntry &ent)
{
    if ( sQueue.empty() )
    {
        gIdleCallbacks.addFunction(onIdle, NULL);
    }

    sQueue.push_back(ent);
}

//static 
void LLDelayedGestureError::onIdle(void *userdata)
{
    if ( ! sQueue.empty() )
    {
        LLErrorEntry ent = sQueue.front();
        sQueue.pop_front();

        if ( ! doDialog(ent, false ) )
        {
            enqueue(ent);
        }
    }
    else
    {
        // Nothing to do anymore
        gIdleCallbacks.deleteFunction(onIdle, NULL);
    }
}

//static 
bool LLDelayedGestureError::doDialog(const LLErrorEntry &ent, bool uuid_ok)
{
    LLSD args;
    LLInventoryItem *item = gInventory.getItem( ent.mItemID );

    if ( item )
    {
        args["NAME"] = item->getName();
    }
    else
    {
        if ( uuid_ok || ent.mTimer.getElapsedTimeF32() > MAX_NAME_WAIT_TIME )
        {
            args["NAME"] = ent.mItemID.asString();
        }
        else
        {
            return false;
        }
    }
     
    if(!LLApp::isExiting())
    {
        LLNotificationsUtil::add(ent.mNotifyName, args);
    }
    
    return true;
}

