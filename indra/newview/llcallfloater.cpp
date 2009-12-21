/** 
 * @file llcallfloater.cpp
 * @author Mike Antipov
 * @brief Voice Control Panel in a Voice Chats (P2P, Group, Nearby...).
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llnotificationsutil.h"
#include "lltrans.h"

#include "llcallfloater.h"

#include "llagent.h"
#include "llagentdata.h" // for gAgentID
#include "llavatarlist.h"
#include "llbottomtray.h"
#include "llimfloater.h"
#include "llfloaterreg.h"
#include "llparticipantlist.h"
#include "llspeakers.h"
#include "lltransientfloatermgr.h"


class LLNonAvatarCaller : public LLAvatarListItem
{
public:
	LLNonAvatarCaller() : LLAvatarListItem(false)
	{

	}
	BOOL postBuild()
	{
		BOOL rv = LLAvatarListItem::postBuild();

		if (rv)
		{
			setOnline(true);
			showLastInteractionTime(false);
			setShowProfileBtn(false);
			setShowInfoBtn(false);
		}
		return rv;
	}

	void setSpeakerId(const LLUUID& id) { mSpeakingIndicator->setSpeakerId(id); }
};


static void* create_non_avatar_caller(void*)
{
	return new LLNonAvatarCaller;
}

LLCallFloater::LLCallFloater(const LLSD& key)
: LLDockableFloater(NULL, false, key)
, mSpeakerManager(NULL)
, mPaticipants(NULL)
, mAvatarList(NULL)
, mNonAvatarCaller(NULL)
, mVoiceType(VC_LOCAL_CHAT)
, mAgentPanel(NULL)
, mSpeakingIndicator(NULL)
, mIsModeratorMutedVoice(false)
{
	mFactoryMap["non_avatar_caller"] = LLCallbackMap(create_non_avatar_caller, NULL);
	LLVoiceClient::getInstance()->addObserver(this);
	LLTransientFloaterMgr::getInstance()->addControlView(this);
}

LLCallFloater::~LLCallFloater()
{
	delete mPaticipants;
	mPaticipants = NULL;

	// Don't use LLVoiceClient::getInstance() here 
	// singleton MAY have already been destroyed.
	if(gVoiceClient)
	{
		gVoiceClient->removeObserver(this);
	}
	LLTransientFloaterMgr::getInstance()->removeControlView(this);
}

// virtual
BOOL LLCallFloater::postBuild()
{
	LLDockableFloater::postBuild();
	mAvatarList = getChild<LLAvatarList>("speakers_list");
	childSetAction("leave_call_btn", boost::bind(&LLCallFloater::leaveCall, this));

	mNonAvatarCaller = getChild<LLNonAvatarCaller>("non_avatar_caller");

	LLView *anchor_panel = LLBottomTray::getInstance()->getChild<LLView>("speak_panel");

	setDockControl(new LLDockControl(
		anchor_panel, this,
		getDockTongue(), LLDockControl::TOP));

	initAgentData();

	// update list for current session
	updateSession();

	return TRUE;
}

// virtual
void LLCallFloater::onOpen(const LLSD& /*key*/)
{
}

// virtual
void LLCallFloater::draw()
{
	// we have to refresh participants to display ones not in voice as disabled.
	// It should be done only when she joins or leaves voice chat.
	// But seems that LLVoiceClientParticipantObserver is not enough to satisfy this requirement.
	// *TODO: mantipov: remove from draw()
	onChange();

	bool is_moderator_muted = gVoiceClient->getIsModeratorMuted(gAgentID);

	if (mIsModeratorMutedVoice != is_moderator_muted)
	{
		setModeratorMutedVoice(is_moderator_muted);
	}

	LLDockableFloater::draw();
}

// virtual
void LLCallFloater::onChange()
{
	if (NULL == mPaticipants) return;

	mPaticipants->refreshVoiceState();
}


//////////////////////////////////////////////////////////////////////////
/// PRIVATE SECTION
//////////////////////////////////////////////////////////////////////////

void LLCallFloater::leaveCall()
{
	LLVoiceChannel* voice_channel = LLVoiceChannel::getCurrentVoiceChannel();
	if (voice_channel)
	{
		voice_channel->deactivate();
	}
}

void LLCallFloater::updateSession()
{
	LLVoiceChannel* voice_channel = LLVoiceChannel::getCurrentVoiceChannel();
	if (voice_channel)
	{
		lldebugs << "Current voice channel: " << voice_channel->getSessionID() << llendl;

		if (mSpeakerManager && voice_channel->getSessionID() == mSpeakerManager->getSessionID())
		{
			lldebugs << "Speaker manager is already set for session: " << voice_channel->getSessionID() << llendl;
			return;
		}
		else
		{
			mSpeakerManager = NULL;
		}
	}

	const LLUUID& session_id = voice_channel->getSessionID();
	lldebugs << "Set speaker manager for session: " << session_id << llendl;

	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(session_id);
	if (im_session)
	{
		mSpeakerManager = LLIMModel::getInstance()->getSpeakerManager(session_id);
		switch (im_session->mType)
		{
		case IM_NOTHING_SPECIAL:
		case IM_SESSION_P2P_INVITE:
			mVoiceType = VC_PEER_TO_PEER;
			break;
		case IM_SESSION_CONFERENCE_START:
		case IM_SESSION_GROUP_START:
		case IM_SESSION_INVITE:
			if (gAgent.isInGroup(session_id))
			{
				mVoiceType = VC_GROUP_CHAT;
			}
			else
			{
				mVoiceType = VC_AD_HOC_CHAT;				
			}
			break;
		default:
			llwarning("Failed to determine voice call IM type", 0);
			mVoiceType = VC_GROUP_CHAT;
			break;
		}
	}

	if (NULL == mSpeakerManager)
	{
		// by default let show nearby chat participants
		mSpeakerManager = LLLocalSpeakerMgr::getInstance();
		lldebugs << "Set DEFAULT speaker manager" << llendl;
		mVoiceType = VC_LOCAL_CHAT;
	}

	updateTitle();
	
	//hide "Leave Call" button for nearby chat
	bool is_local_chat = mVoiceType == VC_LOCAL_CHAT;
	childSetVisible("leave_call_btn", !is_local_chat);
	
	refreshPartisipantList();
	updateModeratorState();

	//show floater for voice calls
	if (!is_local_chat)
	{
		LLIMFloater* im_floater = LLIMFloater::findInstance(session_id);
		bool show_me = !(im_floater && im_floater->getVisible());
		if (show_me) 
		{
			setVisible(true);
		}
	}
}

void LLCallFloater::refreshPartisipantList()
{
	delete mPaticipants;
	mPaticipants = NULL;
	mAvatarList->clear();

	bool non_avatar_caller = false;
	if (VC_PEER_TO_PEER == mVoiceType)
	{
		LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(mSpeakerManager->getSessionID());
		non_avatar_caller = !session->mOtherParticipantIsAvatar;
		if (non_avatar_caller)
		{
			mNonAvatarCaller->setSpeakerId(session->mOtherParticipantID);
			mNonAvatarCaller->setName(session->mName);
		}
	}

	mNonAvatarCaller->setVisible(non_avatar_caller);
	mAvatarList->setVisible(!non_avatar_caller);

	if (!non_avatar_caller)
	{
		mPaticipants = new LLParticipantList(mSpeakerManager, mAvatarList, true, mVoiceType != VC_GROUP_CHAT && mVoiceType != VC_AD_HOC_CHAT);

		if (LLLocalSpeakerMgr::getInstance() == mSpeakerManager)
		{
			mAvatarList->setNoItemsCommentText(getString("no_one_near"));
		}
		mPaticipants->refreshVoiceState();	
	}
}

void LLCallFloater::sOnCurrentChannelChanged(const LLUUID& /*session_id*/)
{
	// Don't update participant list if no channel info is available.
	// Fix for ticket EXT-3427
	// @see LLParticipantList::~LLParticipantList()
	if(LLVoiceChannel::getCurrentVoiceChannel() && 
		LLVoiceChannel::STATE_NO_CHANNEL_INFO == LLVoiceChannel::getCurrentVoiceChannel()->getState())
	{
		return;
	}
	LLCallFloater* call_floater = LLFloaterReg::getTypedInstance<LLCallFloater>("voice_controls");

	// Forget speaker manager from the previous session to avoid using it after session was destroyed.
	call_floater->mSpeakerManager = NULL;
	call_floater->updateSession();
}

void LLCallFloater::updateTitle()
{
	LLVoiceChannel* voice_channel = LLVoiceChannel::getCurrentVoiceChannel();
	std::string title;
	switch (mVoiceType)
	{
	case VC_LOCAL_CHAT:
		title = getString("title_nearby");
		break;
	case VC_PEER_TO_PEER:
		{
			LLStringUtil::format_map_t args;
			args["[NAME]"] = voice_channel->getSessionName();
			title = getString("title_peer_2_peer", args);
		}
		break;
	case VC_AD_HOC_CHAT:
		title = getString("title_adhoc");
		break;
	case VC_GROUP_CHAT:
		{
			LLStringUtil::format_map_t args;
			args["[GROUP]"] = voice_channel->getSessionName();
			title = getString("title_group", args);
		}
		break;
	}

	setTitle(title);
}

void LLCallFloater::initAgentData()
{
	mAgentPanel = getChild<LLPanel> ("my_panel");

	if ( mAgentPanel )
	{
		mAgentPanel->childSetValue("user_icon", gAgentID);

		std::string name;
		gCacheName->getFullName(gAgentID, name);
		mAgentPanel->childSetValue("user_text", name);

		mSpeakingIndicator = mAgentPanel->getChild<LLOutputMonitorCtrl>("speaking_indicator");
		mSpeakingIndicator->setSpeakerId(gAgentID);
	}
}

void LLCallFloater::setModeratorMutedVoice(bool moderator_muted)
{
	mIsModeratorMutedVoice = moderator_muted;

	if (moderator_muted)
	{
		LLNotificationsUtil::add("VoiceIsMutedByModerator");
	}
	mSpeakingIndicator->setIsMuted(moderator_muted);
}

void LLCallFloater::updateModeratorState()
{
	std::string name;
	gCacheName->getFullName(gAgentID, name);

	if(gAgent.isInGroup(mSpeakerManager->getSessionID()))
	{
		// This method can be called when LLVoiceChannel.mState == STATE_NO_CHANNEL_INFO
		// in this case there are no any speakers yet.
		if (mSpeakerManager->findSpeaker(gAgentID))
		{
			// Agent is Moderator
			if (mSpeakerManager->findSpeaker(gAgentID)->mIsModerator)

			{
				const std::string moderator_indicator(LLTrans::getString("IM_moderator_label")); 
				name += " " + moderator_indicator;
			}
		}
	}
	mAgentPanel->childSetValue("user_text", name);
}
//EOF
