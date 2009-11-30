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

#include "llcallfloater.h"

#include "llavatarlist.h"
#include "llbottomtray.h"
#include "llparticipantlist.h"
#include "llspeakers.h"


LLCallFloater::LLCallFloater(const LLSD& key)
: LLDockableFloater(NULL, key)
, mSpeakerManager(NULL)
, mPaticipants(NULL)
, mAvatarList(NULL)
{

}

LLCallFloater::~LLCallFloater()
{
	delete mPaticipants;
	mPaticipants = NULL;
}

// virtual
BOOL LLCallFloater::postBuild()
{
	LLDockableFloater::postBuild();
	mAvatarList = getChild<LLAvatarList>("speakers_list");


	LLView *anchor_panel = LLBottomTray::getInstance()->getChild<LLView>("speak_panel");

	setDockControl(new LLDockControl(
		anchor_panel, this,
		getDockTongue(), LLDockControl::TOP));

	// update list for current session
	updateSession();

	// subscribe to to be notified Voice Channel is changed
	LLVoiceChannel::setCurrentVoiceChannelChangedCallback(boost::bind(&LLCallFloater::onCurrentChannelChanged, this, _1));
	return TRUE;
}

// virtual
void LLCallFloater::onOpen(const LLSD& /*key*/)
{
}

//////////////////////////////////////////////////////////////////////////
/// PRIVATE SECTION
//////////////////////////////////////////////////////////////////////////
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
	}

	if (NULL == mSpeakerManager)
	{
		// by default let show nearby chat participants
		mSpeakerManager = LLLocalSpeakerMgr::getInstance();
		lldebugs << "Set DEFAULT speaker manager" << llendl;
	}

	updateTitle();
	refreshPartisipantList();
}

void LLCallFloater::refreshPartisipantList()
{
	delete mPaticipants;
	mAvatarList->clear();

	bool do_not_use_context_menu_in_local_chat = LLLocalSpeakerMgr::getInstance() != mSpeakerManager;
	mPaticipants = new LLParticipantList(mSpeakerManager, mAvatarList, do_not_use_context_menu_in_local_chat);
}

void LLCallFloater::onCurrentChannelChanged(const LLUUID& /*session_id*/)
{
	updateSession();
}

void LLCallFloater::updateTitle()
{
	LLVoiceChannel* voice_channel = LLVoiceChannel::getCurrentVoiceChannel();
	if (NULL == voice_channel) return;

	std::string title = voice_channel->getSessionName();

	if (LLLocalSpeakerMgr::getInstance() == mSpeakerManager)
	{
		title = getString("title_nearby");
	}

	// *TODO: mantipov: update code to use title from xml for other chat types
	setTitle(title);
}
//EOF
