/** 
 * @file llpanelimcontrolpanel.h
 * @brief LLPanelIMControlPanel and related class definitions
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

#ifndef LL_LLPANELIMCONTROLPANEL_H
#define LL_LLPANELIMCONTROLPANEL_H

#include "llpanel.h"
#include "llcallingcard.h"

class LLParticipantList;

class LLPanelChatControlPanel
	: public LLPanel
{
public:
	LLPanelChatControlPanel() :
		mSessionId(LLUUID()) {};
	~LLPanelChatControlPanel();

	virtual BOOL postBuild();

	virtual void setSessionId(const LLUUID& session_id);
	const LLUUID& getSessionId() { return mSessionId; }

private:
	LLUUID mSessionId;

	// connection to voice channel state change signal
	boost::signals2::connection mVoiceChannelStateChangeConnection;
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
};

class LLPanelAdHocControlPanel : public LLPanelGroupControlPanel
{
public:
	LLPanelAdHocControlPanel(const LLUUID& session_id);

	BOOL postBuild();

};

#endif // LL_LLPANELIMCONTROLPANEL_H
