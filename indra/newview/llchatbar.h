/** 
 * @file llchatbar.h
 * @brief LLChatBar class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
	static void	onClickSay( LLUICtrl*, void* userdata );

	static void	onTabClick( void* userdata );
	static void	onInputEditorKeystroke(LLLineEditor* caller, void* userdata);
	static void	onInputEditorFocusLost(LLFocusableElement* caller,void* userdata);
	static void	onInputEditorGainFocus(LLFocusableElement* caller,void* userdata);

	static void onCommitGesture(LLUICtrl* ctrl, void* data);

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
