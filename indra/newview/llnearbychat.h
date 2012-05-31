/** 
 * @file llnearbychat.h
 * @brief LLNearbyChat class definition
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

#ifndef LL_LLNEARBYCHAT_H
#define LL_LLNEARBYCHAT_H

#include "llimconversation.h"
#include "llcombobox.h"
#include "llgesturemgr.h"
#include "llchat.h"
#include "llvoiceclient.h"
#include "lloutputmonitorctrl.h"
#include "llspeakers.h"
#include "llscrollbar.h"
#include "llviewerchat.h"
#include "llpanel.h"

class LLResizeBar;
class LLChatHistory;

class LLNearbyChat
	:	public LLIMConversation
{
public:
	// constructor for inline chat-bars (e.g. hosted in chat history window)
	LLNearbyChat(const LLSD& key);
	~LLNearbyChat() {}

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);

	// focus overrides
	/*virtual*/ void	onFocusLost();
	/*virtual*/ void	onFocusReceived();

	/*virtual*/ void	setVisible(BOOL visible);

	void loadHistory();
    void reloadMessages();
	void removeScreenChat();

	static LLNearbyChat* getInstance();

	void addToHost();

	/** @param archive true - to save a message to the chat history log */
	void	addMessage			(const LLChat& message,bool archive = true, const LLSD &args = LLSD());
	void	onNearbyChatContextMenuItemClicked(const LLSD& userdata);
	bool	onNearbyChatCheckContextMenuItem(const LLSD& userdata);

	LLLineEditor* getChatBox() { return mChatBox; }

	//virtual void draw();

	std::string getCurrentChat();

	virtual BOOL handleKeyHere( KEY key, MASK mask );
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);

	static void startChat(const char* line);
	static void stopChat();

	static void sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate);
	static void sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate);

	void showHistory();
	void showTranslationCheckbox(BOOL show);

protected:
	static BOOL matchChatTypeTrigger(const std::string& in_str, std::string* out_str);
	static void onChatBoxKeystroke(LLLineEditor* caller, void* userdata);
	static void onChatBoxFocusLost(LLFocusableElement* caller, void* userdata);
	void onChatBoxFocusReceived();

	void sendChat( EChatType type );
	void onChatBoxCommit();
	void onChatFontChange(LLFontGL* fontp);

	/* virtual */ bool applyRectControl();

	void onToggleNearbyChatPanel();

	static LLWString stripChannelNumber(const LLWString &mesg, S32* channel);
	EChatType processChatTypeTriggers(EChatType type, std::string &str);

	void displaySpeakingIndicator();

	void onCallButtonClicked();

	// set the enable/disable state for the Call button
	virtual void enableDisableCallBtn();

	// Which non-zero channel did we last chat on?
	static S32 sLastSpecialChatChannel;

	LLLineEditor*			mChatBox;
	LLOutputMonitorCtrl*	mOutputMonitor;
	LLLocalSpeakerMgr*		mSpeakerMgr;

	S32 mExpandedHeight;

	/*virtual*/ BOOL tick();

private:

	void	getAllowedRect		(LLRect& rect);
	// prepare chat's params and out one message to chatHistory
	void appendMessage(const LLChat& chat, const LLSD &args = 0);
	void	onNearbySpeakers	();

	LLHandle<LLView>	mPopupMenuHandle;
	std::vector<LLChat> mMessageArchive;
	LLChatHistory*		mChatHistory;

};

#endif
