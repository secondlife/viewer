/** 
 * @file llviewergesture.cpp
 * @brief LLViewerGesture class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llviewergesture.h"

#include "audioengine.h"
#include "lldir.h"
#include "llviewerinventory.h"
#include "sound_ids.h"		// for testing

#include "llchatbar.h"
#include "llkeyboard.h"		// for key shortcuts for testing
#include "llinventorymodel.h"
#include "llvoavatar.h"
#include "llxfermanager.h"
#include "llviewermessage.h" // send_guid_sound_trigger
#include "llviewernetwork.h"
#include "llagent.h"

// Globals
LLViewerGestureList gGestureList;

const F32 LLViewerGesture::SOUND_VOLUME = 1.f;

LLViewerGesture::LLViewerGesture()
:	LLGesture()
{ }

LLViewerGesture::LLViewerGesture(KEY key, MASK mask, const std::string &trigger,
								 const LLUUID &sound_item_id, 
								 const std::string &animation,
								 const std::string &output_string)
:	LLGesture(key, mask, trigger, sound_item_id, animation, output_string)
{
}

LLViewerGesture::LLViewerGesture(U8 **buffer, S32 max_size)
:	LLGesture(buffer, max_size)
{
}

LLViewerGesture::LLViewerGesture(const LLViewerGesture &rhs)
:	LLGesture((LLGesture)rhs)
{
}

BOOL LLViewerGesture::trigger(KEY key, MASK mask)
{
	if (mKey == key && mMask == mask)
	{
		doTrigger( TRUE );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


BOOL LLViewerGesture::trigger(const std::string &trigger_string)
{
	// Assumes trigger_string is lowercase
	if (mTriggerLower == trigger_string)
	{
		doTrigger( FALSE );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


// private
void LLViewerGesture::doTrigger( BOOL send_chat )
{
	if (mSoundItemID != LLUUID::null)
	{
		LLViewerInventoryItem *item;
		item = gInventory.getItem(mSoundItemID);
		if (item)
		{
			send_sound_trigger(item->getAssetUUID(), SOUND_VOLUME);
		}
	}

	if (!mAnimation.empty())
	{
		// AFK animations trigger the special "away" state, which
		// includes agent control settings. JC
		if (mAnimation == "enter_away_from_keyboard_state" || mAnimation == "away")
		{
			gAgent.setAFK();
		}
		else
		{
			LLUUID anim_id = gAnimLibrary.stringToAnimState(mAnimation);
			gAgent.sendAnimationRequest(anim_id, ANIM_REQUEST_START);
		}
	}

	if ( send_chat && !mOutputString.empty())
	{
		// Don't play nodding animation, since that might not blend
		// with the gesture animation.
		gChatBar->sendChatFromViewer(mOutputString, CHAT_TYPE_NORMAL, FALSE);
	}
}


LLViewerGestureList::LLViewerGestureList()
:	LLGestureList()
{
	mIsLoaded = FALSE;
}


// helper for deserialize that creates the right LLGesture subclass
LLGesture *LLViewerGestureList::create_gesture(U8 **buffer, S32 max_size)
{
	return new LLViewerGesture(buffer, max_size);
}


// See if the prefix matches any gesture.  If so, return TRUE
// and place the full text of the gesture trigger into
// output_str
BOOL LLViewerGestureList::matchPrefix(const std::string& in_str, std::string* out_str)
{
	S32 in_len = in_str.length();

	std::string in_str_lc = in_str;
	LLStringUtil::toLower(in_str_lc);

	for (S32 i = 0; i < count(); i++)
	{
		LLGesture* gesture = get(i);
		const std::string &trigger = gesture->getTrigger();
		
		if (in_len > (S32)trigger.length())
		{
			// too short, bail out
			continue;
		}

		std::string trigger_trunc = utf8str_truncate(trigger, in_len);
		LLStringUtil::toLower(trigger_trunc);
		if (in_str_lc == trigger_trunc)
		{
			*out_str = trigger;
			return TRUE;
		}
	}
	return FALSE;
}


// static
void LLViewerGestureList::xferCallback(void *data, S32 size, void** /*user_data*/, S32 status)
{
	if (LL_ERR_NOERR == status)
	{
		U8 *buffer = (U8 *)data;
		U8 *end = gGestureList.deserialize(buffer, size);

		if (end - buffer > size)
		{
			llerrs << "Read off of end of array, error in serialization" << llendl;
		}

		gGestureList.mIsLoaded = TRUE;
	}
	else
	{
		llwarns << "Unable to load gesture list!" << llendl;
	}
}
