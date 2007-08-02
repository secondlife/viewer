/** 
 * @file llfloaterchat.h
 * @brief LLFloaterChat class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/*
 * Actually the "Chat History" floater.
 * Should be llfloaterchathistory, not llfloaterchat.
 */

#ifndef LL_LLFLOATERCHAT_H
#define LL_LLFLOATERCHAT_H

#include "llfloater.h"

class LLButton;
class LLChat;
class LLComboBox;
class LLLineEditor;
class LLViewerTextEditor;
class LLMessageSystem;
class LLUUID;
class LLCheckBoxCtrl;
class LLPanelActiveSpeakers;

class LLFloaterChat
:	public LLFloater, public LLUISingleton<LLFloaterChat>
{
public:
	LLFloaterChat(const LLSD& seed);
	~LLFloaterChat();

	virtual void setVisible( BOOL b );
	virtual void draw();
	virtual BOOL postBuild();
	virtual void onClose(bool app_quitting);
	virtual void onVisibilityChange(BOOL cur_visibility);

	static void setHistoryCursorAndScrollToEnd();
	
	// Add chat to console and history list.
	// Color based on source, type, distance.
	static void addChat(const LLChat& chat, BOOL from_im = FALSE, BOOL local_agent = FALSE);
	
	// Add chat to history alone.
	static void addChatHistory(const LLChat& chat, bool log_to_file = true);
	
	static void onClickMute(void *data);
	static void onClickToggleShowMute(LLUICtrl* caller, void *data);
	static void onClickToggleActiveSpeakers(void* userdata);
	static void chatFromLogFile(LLString line, void* userdata);
	static void loadHistory();
	static void* createSpeakersPanel(void* data);
	static void* createChatPanel(void* data);
	static void hideInstance(const LLSD& id);

protected:
	LLPanelActiveSpeakers* mPanel;
};

#endif
