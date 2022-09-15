/** 
 * @file llparticipantlist.cpp
 * @brief LLParticipantList : model of a conversation session with added speaker events handling
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llavatarnamecache.h"
#include "llerror.h"
#include "llimview.h"
#include "llfloaterimcontainer.h"
#include "llparticipantlist.h"
#include "llspeakers.h"

//LLParticipantList retrieves add, clear and remove events and updates view accordingly 
#if LL_MSVC
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

LLParticipantList::LLParticipantList(LLSpeakerMgr* data_source, LLFolderViewModelInterface& root_view_model) :
	LLConversationItemSession(data_source->getSessionID(), root_view_model),
	mSpeakerMgr(data_source),
	mValidateSpeakerCallback(NULL)
{
	mSpeakerAddListener = new SpeakerAddListener(*this);
	mSpeakerRemoveListener = new SpeakerRemoveListener(*this);
	mSpeakerClearListener = new SpeakerClearListener(*this);
	mSpeakerModeratorListener = new SpeakerModeratorUpdateListener(*this);
	mSpeakerUpdateListener = new SpeakerUpdateListener(*this);
	mSpeakerMuteListener = new SpeakerMuteListener(*this);

	mSpeakerMgr->addListener(mSpeakerAddListener, "add");
	mSpeakerMgr->addListener(mSpeakerRemoveListener, "remove");
	mSpeakerMgr->addListener(mSpeakerClearListener, "clear");
	mSpeakerMgr->addListener(mSpeakerModeratorListener, "update_moderator");
	mSpeakerMgr->addListener(mSpeakerUpdateListener, "update_speaker");

	setSessionID(mSpeakerMgr->getSessionID());

	//Lets fill avatarList with existing speakers
	LLSpeakerMgr::speaker_list_t speaker_list;
	mSpeakerMgr->getSpeakerList(&speaker_list, true);
	for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
	{
		const LLPointer<LLSpeaker>& speakerp = *it;

		addAvatarIDExceptAgent(speakerp->mID);
		if ( speakerp->mIsModerator )
		{
			mModeratorList.insert(speakerp->mID);
		}
		else
		{
			mModeratorToRemoveList.insert(speakerp->mID);
		}
	}
	
	// Identify and store what kind of session we are
	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(data_source->getSessionID());
	if (im_session)
	{
		// By default, sessions that can't be identified as group or ad-hoc will be considered P2P (i.e. 1 on 1)
		mConvType = CONV_SESSION_1_ON_1;
		if (im_session->isAdHocSessionType())
		{
			mConvType = CONV_SESSION_AD_HOC;
		}
		else if (im_session->isGroupSessionType())
		{
			mConvType = CONV_SESSION_GROUP;
		}
	}
	else 
	{
		// That's the only session that doesn't get listed in the LLIMModel as a session...
		mConvType = CONV_SESSION_NEARBY;
	}
}

LLParticipantList::~LLParticipantList()
{
}

void LLParticipantList::setValidateSpeakerCallback(validate_speaker_callback_t cb)
{
	mValidateSpeakerCallback = cb;
}

void LLParticipantList::update()
{
	mSpeakerMgr->update(true);
}

bool LLParticipantList::onAddItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLUUID uu_id = event->getValue().asUUID();

	if (mValidateSpeakerCallback && !mValidateSpeakerCallback(uu_id))
	{
		return true;
	}

	addAvatarIDExceptAgent(uu_id);
	return true;
}

bool LLParticipantList::onRemoveItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLUUID avatar_id = event->getValue().asUUID();
	removeParticipant(avatar_id);
	return true;
}

bool LLParticipantList::onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	clearParticipants();
	return true;
}

bool LLParticipantList::onSpeakerUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	const LLSD& evt_data = event->getValue();
	if ( evt_data.has("id") )
	{
		LLUUID participant_id = evt_data["id"];
		LLFloaterIMContainer* im_box = LLFloaterIMContainer::findInstance();
		if (im_box)
		{
			im_box->setTimeNow(mUUID,participant_id);
		}
	}
	return true;
}

bool LLParticipantList::onModeratorUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	const LLSD& evt_data = event->getValue();
	if ( evt_data.has("id") && evt_data.has("is_moderator") )
	{
		LLUUID id = evt_data["id"];
		bool is_moderator = evt_data["is_moderator"];
		if ( id.notNull() )
		{
			if ( is_moderator )
				mModeratorList.insert(id);
			else
			{
				std::set<LLUUID>::iterator it = mModeratorList.find (id);
				if ( it != mModeratorList.end () )
				{
					mModeratorToRemoveList.insert(id);
					mModeratorList.erase(id);
				}
			}
			// *TODO : do we have to fire an event so that LLFloaterIMSessionTab::refreshConversation() gets called
		}
	}
	return true;
}

bool LLParticipantList::onSpeakerMuteEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLPointer<LLSpeaker> speakerp = (LLSpeaker*)event->getSource();
	if (speakerp.isNull()) return false;

	// update UI on confirmation of moderator mutes
	if (event->getValue().asString() == "voice")
	{
		setParticipantIsMuted(speakerp->mID, speakerp->mModeratorMutedVoice);
	}
	return true;
}

void LLParticipantList::addAvatarIDExceptAgent(const LLUUID& avatar_id)
{
	// Do not add if already in there, is the session id (hence not an avatar) or excluded for some reason
	if (findParticipant(avatar_id) || (avatar_id == mUUID))
	{
		return;
	}

	bool is_avatar = LLVoiceClient::getInstance()->isParticipantAvatar(avatar_id);

	LLConversationItemParticipant* participant = NULL;
	
	if (is_avatar)
	{
		// Create a participant view model instance
		LLAvatarName avatar_name;
		bool has_name = LLAvatarNameCache::get(avatar_id, &avatar_name);
		participant = new LLConversationItemParticipant(!has_name ? LLTrans::getString("AvatarNameWaiting") : avatar_name.getDisplayName() , avatar_id, mRootViewModel);
		participant->fetchAvatarName();
	}
	else
	{
		std::string display_name = LLVoiceClient::getInstance()->getDisplayName(avatar_id);
		// Create a participant view model instance
		participant = new LLConversationItemParticipant(display_name.empty() ? LLTrans::getString("AvatarNameWaiting") : display_name, avatar_id, mRootViewModel);
	}

	// *TODO : Need to update the online/offline status of the participant
	// Hack for this: LLAvatarTracker::instance().isBuddyOnline(avatar_id))
	
	// Add the participant model to the session's children list
	// This will post "add_participant" event
	addParticipant(participant);

	adjustParticipant(avatar_id);
}

static LLFastTimer::DeclareTimer FTM_FOLDERVIEW_TEST("add test avatar agents");

void LLParticipantList::adjustParticipant(const LLUUID& speaker_id)
{
	LLPointer<LLSpeaker> speakerp = mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp.isNull()) return;

	// add listener to process moderation changes
	speakerp->addListener(mSpeakerMuteListener);
}

//
// LLParticipantList::SpeakerAddListener
//
bool LLParticipantList::SpeakerAddListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	/**
	 * We need to filter speaking objects. These objects shouldn't appear in the list.
	 * @see LLFloaterChat::addChat() in llviewermessage.cpp to get detailed call hierarchy
	 */
	const LLUUID& speaker_id = event->getValue().asUUID();
	LLPointer<LLSpeaker> speaker = mParent.mSpeakerMgr->findSpeaker(speaker_id);
	if (speaker.isNull() || (speaker->mType == LLSpeaker::SPEAKER_OBJECT))
	{
		return false;
	}
	return mParent.onAddItemEvent(event, userdata);
}

//
// LLParticipantList::SpeakerRemoveListener
//
bool LLParticipantList::SpeakerRemoveListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onRemoveItemEvent(event, userdata);
}

//
// LLParticipantList::SpeakerClearListener
//
bool LLParticipantList::SpeakerClearListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onClearListEvent(event, userdata);
}

//
// LLParticipantList::SpeakerUpdateListener
//
bool LLParticipantList::SpeakerUpdateListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onSpeakerUpdateEvent(event, userdata);
}

//
// LLParticipantList::SpeakerModeratorListener
//
bool LLParticipantList::SpeakerModeratorUpdateListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onModeratorUpdateEvent(event, userdata);
}

bool LLParticipantList::SpeakerMuteListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onSpeakerMuteEvent(event, userdata);
}

//EOF
