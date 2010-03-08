/** 
 * @file llpanelimcontrolpanel.h
 * @brief LLPanelIMControlPanel and related class definitions
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLPANELIMCONTROLPANEL_H
#define LL_LLPANELIMCONTROLPANEL_H

#include "llpanel.h"
#include "llvoicechannel.h"
#include "llcallingcard.h"

class LLParticipantList;

class LLPanelChatControlPanel 
	: public LLPanel
	, public LLVoiceClientStatusObserver
{
public:
	LLPanelChatControlPanel() :
		mSessionId(LLUUID()) {};
	~LLPanelChatControlPanel();

	virtual BOOL postBuild();

	void onCallButtonClicked();
	void onEndCallButtonClicked();
	void onOpenVoiceControlsClicked();

	// Implements LLVoiceClientStatusObserver::onChange() to enable the call
	// button when voice is available
	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

	virtual void onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state);

	void updateButtons(bool is_call_started);
	
	// Enables/disables call button depending on voice availability
	void updateCallButton();

	virtual void setSessionId(const LLUUID& session_id);
	const LLUUID& getSessionId() { return mSessionId; }

private:
	LLUUID mSessionId;

	// connection to voice channel state change signal
	boost::signals2::connection mVoiceChannelStateChangeConnection;
};


class LLPanelIMControlPanel : public LLPanelChatControlPanel, LLFriendObserver
{
public:
	LLPanelIMControlPanel();
	~LLPanelIMControlPanel();

	BOOL postBuild();

	void setSessionId(const LLUUID& session_id);

	// LLFriendObserver trigger
	virtual void changed(U32 mask);

protected:
	void onNameCache(const LLUUID& id, const std::string& full_name, bool is_group);

private:
	void onViewProfileButtonClicked();
	void onAddFriendButtonClicked();
	void onShareButtonClicked();
	void onTeleportButtonClicked();
	void onPayButtonClicked();

	LLUUID mAvatarID;
};


class LLPanelGroupControlPanel : public LLPanelChatControlPanel
{
public:
	LLPanelGroupControlPanel(const LLUUID& session_id);
	~LLPanelGroupControlPanel();

	BOOL postBuild();

	void setSessionId(const LLUUID& session_id);
	/*virtual*/ void draw();

protected:
	LLUUID mGroupID;

	LLParticipantList* mParticipantList;

private:
	void onGroupInfoButtonClicked();
	void onSortMenuItemClicked(const LLSD& userdata);
	/*virtual*/ void onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state);
};

class LLPanelAdHocControlPanel : public LLPanelGroupControlPanel
{
public:
	LLPanelAdHocControlPanel(const LLUUID& session_id);

	BOOL postBuild();

};

#endif // LL_LLPANELIMCONTROLPANEL_H
