/** 
 * @file lldelayedgestureerror.cpp
 * @brief Delayed gesture error message -- try to wait until name has been retrieved
 * @author Dale Glass
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "lldelayedgestureerror.h"
#include <list>
#include "llnotify.h"
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
	LLStringUtil::format_map_t args;
	LLInventoryItem *item = gInventory.getItem( ent.mItemID );

	if ( item )
	{
		args["[NAME]"] = item->getName();
	}
	else
	{
		if ( uuid_ok || ent.mTimer.getElapsedTimeF32() > MAX_NAME_WAIT_TIME )
		{
			args["[NAME]"] = std::string( ent.mItemID.asString() );
		}
		else
		{
			return false;
		}
	}
	 

	LLNotifyBox::showXml(ent.mNotifyName, args);

	return true;
}

