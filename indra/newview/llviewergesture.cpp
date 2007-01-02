/** 
 * @file llviewergesture.cpp
 * @brief LLViewerGesture class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
			LLUUID anim_id = gAnimLibrary.stringToAnimState(mAnimation.c_str());
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

void LLViewerGestureList::saveToServer()
{
	U8 *buffer = new U8[getMaxSerialSize()];

	U8 *end = serialize(buffer);

	if (end - buffer > getMaxSerialSize())
	{
		llerrs << "Wrote off end of buffer, serial size computation is wrong" << llendl;
	}

	//U64 xfer_id = gXferManager->registerXfer(buffer, end - buffer);
	// write to a file because mem<->mem xfer isn't implemented
	LLUUID random_uuid;
	char filename[LL_MAX_PATH];
	random_uuid.generate();
	random_uuid.toString(filename);
	strcat(filename,".tmp");

	char filename_and_path[LL_MAX_PATH];
	sprintf(filename_and_path, "%s%s%s", 
		gDirUtilp->getTempDir().c_str(), 
		gDirUtilp->getDirDelimiter().c_str(),
		filename);

	FILE *fp = LLFile::fopen(filename_and_path, "wb");

	if (fp)
	{
		fwrite(buffer, end - buffer, 1, fp);
		fclose(fp);

		gMessageSystem->newMessageFast(_PREHASH_GestureUpdate);
		gMessageSystem->nextBlockFast(_PREHASH_AgentBlock);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addStringFast(_PREHASH_Filename, filename);
		gMessageSystem->addBOOLFast(_PREHASH_ToViewer, FALSE);
		gMessageSystem->sendReliable(gUserServer);
	}

	delete[] buffer;
}

/*
void LLViewerGestureList::requestFromServer()
{
	gMessageSystem->newMessageFast(_PREHASH_GestureRequest);
	gMessageSystem->nextBlockFast(_PREHASH_AgentBlock);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, agent_get_id());
	gMessageSystem->addU8("Reset", 0);
	gMessageSystem->sendReliable(gUserServer);
}

void LLViewerGestureList::requestResetFromServer( BOOL is_male )
{
	gMessageSystem->newMessageFast(_PREHASH_GestureRequest);
	gMessageSystem->nextBlockFast(_PREHASH_AgentBlock);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, agent_get_id());
	gMessageSystem->addU8("Reset", is_male ? 1 : 2);
	gMessageSystem->sendReliable(gUserServer);
	mIsLoaded = FALSE;
}
*/

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

	LLString in_str_lc = in_str;
	LLString::toLower(in_str_lc);

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
		LLString::toLower(trigger_trunc);
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

// static
void LLViewerGestureList::processGestureUpdate(LLMessageSystem *msg, void** /*user_data*/)
{
	char remote_filename[MAX_STRING];
	msg->getStringFast(_PREHASH_AgentBlock, _PREHASH_Filename, MAX_STRING, remote_filename);


	gXferManager->requestFile(remote_filename, LL_PATH_CACHE, msg->getSender(), TRUE, xferCallback, NULL,
							  LLXferManager::HIGH_PRIORITY);
}
