/**
 * @file llimconversation.h
 * @brief LLIMConversation class implements the common behavior of LNearbyChatBar
 * @brief and LLIMFloater for hosting both in LLIMContainer
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_IMCONVERSATION_H
#define LL_IMCONVERSATION_H

#include "lllayoutstack.h"
#include "llparticipantlist.h"
#include "lltransientdockablefloater.h"
#include "llviewercontrol.h"
#include "lleventtimer.h"

class LLPanelChatControlPanel;

class LLIMConversation
	: public LLTransientDockableFloater
	, public LLEventTimer
{

public:
	LOG_CLASS(LLIMConversation);

	LLIMConversation(const LLUUID& session_id);
	~LLIMConversation();

	// reload all message with new settings of visual modes
	static void processChatHistoryStyleUpdate();

	/**
	 * Returns true if chat is displayed in multi tabbed floater
	 *         false if chat is displayed in multiple windows
	 */
	static bool isChatMultiTab();

	// LLFloater overrides
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ BOOL postBuild();

protected:

	// callback for click on any items of the visual states menu
	void onIMSessionMenuItemClicked(const LLSD& userdata);

	// callback for check/uncheck of the expanded/collapse mode's switcher
	bool onIMCompactExpandedMenuItemCheck(const LLSD& userdata);

	//
	bool onIMShowModesMenuItemCheck(const LLSD& userdata);
	bool onIMShowModesMenuItemEnable(const LLSD& userdata);
	static void onSlide(LLIMConversation *self);
	void onTearOffClicked();

	// refresh a visual state of the Call button
	void updateCallBtnState(bool callIsActive);

	// set the enable/disable state for the Call button
	virtual void enableDisableCallBtn() = 0;

	void buildParticipantList();
	void onSortMenuItemClicked(const LLSD& userdata);

	/*virtual*/ BOOL tick();

	bool mIsNearbyChat;
	bool mIsP2PChat;

	LLLayoutPanel* mParticipantListPanel;
	LLParticipantList* mParticipantList;
	LLUUID mSessionID;

	LLButton* mExpandCollapseBtn;
	LLButton* mTearOffBtn;
	LLButton* mCloseBtn;

private:
	/// Update floater header and toolbar buttons when hosted/torn off state is toggled.
	void updateHeaderAndToolbar();
};


#endif // LL_IMCONVERSATION_H
