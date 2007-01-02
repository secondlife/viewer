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

class LLFloaterChat
:	public LLFloater
{
public:
	LLFloaterChat();
	~LLFloaterChat();

	void show();
	virtual void onClose(bool app_quitting);
	virtual void setVisible( BOOL b );

	static void setHistoryCursorAndScrollToEnd();
	
	// Add chat to console and history list.
	// Color based on source, type, distance.
	static void addChat(const LLChat& chat, BOOL from_im = FALSE, BOOL local_agent = FALSE);
	
	// Add chat to history alone.
	static void addChatHistory(const LLChat& chat, bool log_to_file = true);
	
	static void toggle(void*);
	static BOOL visible(void*);

	static void onClickMute(void *data);
	static void onClickChat(void *);
	static void onCommitUserSelect(LLUICtrl* caller, void* data);
	static void onClickToggleShowMute(LLUICtrl* caller, void *data);
	static void chatFromLogFile(LLString line, void* userdata);
	static void loadHistory();
};

extern LLFloaterChat* gFloaterChat;

#endif
