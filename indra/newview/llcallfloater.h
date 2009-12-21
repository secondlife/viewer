/** 
 * @file llcallfloater.h
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

#ifndef LL_LLCALLFLOATER_H
#define LL_LLCALLFLOATER_H

#include "lldockablefloater.h"
#include "llvoiceclient.h"

class LLAvatarList;
class LLNonAvatarCaller;
class LLOutputMonitorCtrl;
class LLParticipantList;
class LLSpeakerMgr;
/**
 * The Voice Control Panel is an ambient window summoned by clicking the flyout chevron on the Speak button.
 * It can be torn-off and freely positioned onscreen.
 *
 * When the Resident is engaged in Nearby Voice Chat, the Voice Control Panel provides control over 
 * the Resident's own microphone input volume, the audible volume of each of the other participants,
 * the Resident's own Voice Morphing settings (if she has subscribed to enable the feature), and Voice Recording.
 *
 * When the Resident is engaged in any chat except Nearby Chat, the Voice Control Panel also provides an 
 * 'Leave Call' button to allow the Resident to leave that voice channel.
 */
class LLCallFloater : public LLDockableFloater, LLVoiceClientParticipantObserver
{
public:
	LLCallFloater(const LLSD& key);
	~LLCallFloater();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void draw();

	/**
	 * Is called by LLVoiceClient::notifyParticipantObservers when voice participant list is changed.
	 *
	 * Refreshes list to display participants not in voice as disabled.
	 */
	/*virtual*/ void onChange();

	static void sOnCurrentChannelChanged(const LLUUID& session_id);

private:
	typedef enum e_voice_controls_type
	{
		VC_LOCAL_CHAT,
		VC_GROUP_CHAT,
		VC_AD_HOC_CHAT,
		VC_PEER_TO_PEER
	}EVoiceControls;

	void leaveCall();

	/**
	 * Updates mSpeakerManager and list according to current Voice Channel
	 *
	 * It compares mSpeakerManager & current Voice Channel session IDs.
	 * If they are different gets Speaker manager related to current channel and updates channel participant list.
	 */
	void updateSession();

	/**
	 * Refreshes participant list according to current Voice Channel
	 */
	void refreshPartisipantList();


	
	void updateTitle();
	void initAgentData();
	void setModeratorMutedVoice(bool moderator_muted);
	void updateAgentModeratorState();

private:
	LLSpeakerMgr* mSpeakerManager;
	LLParticipantList* mPaticipants;
	LLAvatarList* mAvatarList;
	LLNonAvatarCaller* mNonAvatarCaller;
	EVoiceControls mVoiceType;
	LLPanel* mAgentPanel;
	LLOutputMonitorCtrl* mSpeakingIndicator;
	bool mIsModeratorMutedVoice;
};


#endif //LL_LLCALLFLOATER_H

