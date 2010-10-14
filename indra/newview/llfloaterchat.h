/** 
 * @file llfloaterchat.h
 * @brief LLFloaterChat class definition
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

/*
 * Actually the "Chat History" floater.
 * Should be llfloaterchathistory, not llfloaterchat.
 */

#ifndef LL_LLFLOATERCHAT_H
#define LL_LLFLOATERCHAT_H

#include "llfloater.h"
#include "lllogchat.h"

class LLChat;
class LLPanelActiveSpeakers;
class LLLogChat;

class LLFloaterChat : public LLFloater
{
public:
	LLFloaterChat(const LLSD& seed);
	~LLFloaterChat();

	virtual void draw();
	virtual BOOL postBuild();

	void updateConsoleVisibility();

	static void setHistoryCursorAndScrollToEnd();

	//  *TODO:Skinning - move these to LLChat (or LLViewerChat?)
	// Add chat to console and history list.
	// Color based on source, type, distance.
	static void addChat(const LLChat& chat, BOOL local_agent = FALSE);
	// Add chat to history alone.
	static void addChatHistory(const LLChat& chat, bool log_to_file = true);
	
	static void triggerAlerts(const std::string& text);

	static void onClickMute(void *data);
	static void onClickToggleShowMute(LLUICtrl* caller, void *data);
	static void onClickToggleActiveSpeakers(void* userdata);
	static void chatFromLogFile(LLLogChat::ELogLineType type, const LLSD& line, void* userdata);
	static void loadHistory();
	static void* createSpeakersPanel(void* data);
	static void* createChatPanel(void* data);
	
	static LLFloaterChat* getInstance(); // *TODO:Skinning Deprecate
	
	LLPanelActiveSpeakers* mPanel;
	BOOL mScrolledToEnd;
};

#endif
