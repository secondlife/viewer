/** 
 * @file llviewergesture.cpp
 * @brief LLViewerGesture class implementation
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

#include "llviewergesture.h"

#include "llaudioengine.h"
#include "lldir.h"
#include "llviewerinventory.h"
#include "sound_ids.h"		// for testing

#include "llfloaterreg.h"
#include "llkeyboard.h"		// for key shortcuts for testing
#include "llinventorymodel.h"
#include "llvoavatar.h"
#include "llxfermanager.h"
#include "llviewermessage.h" // send_guid_sound_trigger
#include "llviewernetwork.h"
#include "llagent.h"
#include "llfloaterimnearbychat.h"

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

bool LLViewerGesture::trigger(KEY key, MASK mask)
{
	if (mKey == key && mMask == mask)
	{
		doTrigger( true );
		return true;
	}
	else
	{
		return false;
	}
}


bool LLViewerGesture::trigger(const std::string &trigger_string)
{
	// Assumes trigger_string is lowercase
	if (mTriggerLower == trigger_string)
	{
		doTrigger( false );
		return true;
	}
	else
	{
		return false;
	}
}


// private
void LLViewerGesture::doTrigger( bool send_chat )
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

	if (send_chat && !mOutputString.empty())
	{
		// Don't play nodding animation, since that might not blend
		// with the gesture animation.
		(LLFloaterReg::getTypedInstance<LLFloaterIMNearbyChat>("nearby_chat"))->
				sendChatFromViewer(mOutputString, CHAT_TYPE_NORMAL, false);
	}
}


LLViewerGestureList::LLViewerGestureList()
:	LLGestureList()
{
	mIsLoaded = false;
}


// helper for deserialize that creates the right LLGesture subclass
LLGesture *LLViewerGestureList::create_gesture(U8 **buffer, S32 max_size)
{
	return new LLViewerGesture(buffer, max_size);
}


// See if the prefix matches any gesture.  If so, return true
// and place the full text of the gesture trigger into
// output_str
bool LLViewerGestureList::matchPrefix(const std::string& in_str, std::string* out_str)
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
			return true;
		}
	}
	return false;
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
			LL_ERRS() << "Read off of end of array, error in serialization" << LL_ENDL;
		}

		gGestureList.mIsLoaded = true;
	}
	else
	{
		LL_WARNS() << "Unable to load gesture list!" << LL_ENDL;
	}
}
