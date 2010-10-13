/** 
 * @file llchatbar.h
 * @brief LLChatBar class definition
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

#ifndef LL_LLCHATBAR_H
#define LL_LLCHATBAR_H

#include "llpanel.h"
#include "llframetimer.h"
#include "llchat.h"

class LLLineEditor;
class LLMessageSystem;
class LLUICtrl;
class LLUUID;
class LLFrameTimer;
class LLChatBarGestureObserver;
class LLComboBox;


class LLChatBar
:	public LLPanel
{
public:
	// constructor for inline chat-bars (e.g. hosted in chat history window)
	LLChatBar();
	~LLChatBar();
	virtual BOOL postBuild();

	virtual BOOL handleKeyHere(KEY key, MASK mask);

	void		refresh();
	void		refreshGestures();

	// Move cursor into chat input field.
	void		setKeyboardFocus(BOOL b);

	// Ignore arrow keys for chat bar
	void		setIgnoreArrowKeys(BOOL b);

	BOOL		inputEditorHasFocus();
	std::string	getCurrentChat();

	// since chat bar logic is reused for chat history
	// gesture combo box might not be a direct child
	void		setGestureCombo(LLComboBox* combo);

	// Send a chat (after stripping /20foo channel chats).
	// "Animate" means the nodding animation for regular text.
	void		sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate);
	void		sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate);

	// If input of the form "/20foo" or "/20 foo", returns "foo" and channel 20.
	// Otherwise returns input and channel 0.
	LLWString stripChannelNumber(const LLWString &mesg, S32* channel);

	// callbacks
	void onClickSay(LLUICtrl* ctrl);

	static void	onTabClick( void* userdata );
	static void	onInputEditorKeystroke(LLLineEditor* caller, void* userdata);
	static void	onInputEditorFocusLost();
	static void	onInputEditorGainFocus();

	void onCommitGesture(LLUICtrl* ctrl);

	static void startChat(const char* line);
	static void stopChat();

protected:
	void sendChat(EChatType type);
	void updateChat();

protected:
	LLLineEditor*	mInputEditor;

	LLFrameTimer	mGestureLabelTimer;

	// Which non-zero channel did we last chat on?
	S32				mLastSpecialChatChannel;

	BOOL			mIsBuilt;
	LLComboBox*		mGestureCombo;

	LLChatBarGestureObserver* mObserver;
};

extern LLChatBar *gChatBar;

#endif
