/** 
 * @file llnearbychatbar.h
 * @brief LLNearbyChatBar class definition
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

#ifndef LL_LLNEARBYCHATBAR_H
#define LL_LLNEARBYCHATBAR_H

#include "llpanel.h"
#include "llcombobox.h"
#include "llgesturemgr.h"
#include "llchat.h"
#include "llchiclet.h"
#include "llvoiceclient.h"

class LLGestureComboBox
	: public LLComboBox
	, public LLGestureManagerObserver
{
public:
	struct Params : public LLInitParam::Block<Params, LLComboBox::Params> { };
protected:
	LLGestureComboBox(const Params&);
	friend class LLUICtrlFactory;
public:
	~LLGestureComboBox();

	void refreshGestures();
	void onCommitGesture();
	virtual void draw();

	// LLGestureManagerObserver trigger
	virtual void changed() { refreshGestures(); }

protected:
	LLFrameTimer mGestureLabelTimer;
	std::vector<LLMultiGesture*> mGestures;
	std::string mLabel;
};

class LLNearbyChatBar
:	public LLPanel
,   public LLVoiceClientStatusObserver
{
public:
	// constructor for inline chat-bars (e.g. hosted in chat history window)
	LLNearbyChatBar();
	~LLNearbyChatBar() {}

	virtual BOOL postBuild();

	static LLNearbyChatBar* getInstance();

	static bool instanceExists();

	LLLineEditor* getChatBox() { return mChatBox; }

	std::string getCurrentChat();
	virtual BOOL handleKeyHere( KEY key, MASK mask );

	void setPTTState(bool state);
	static void startChat(const char* line);
	static void stopChat();

	static void sendChatFromViewer(const std::string &utf8text, EChatType type, BOOL animate);
	static void sendChatFromViewer(const LLWString &wtext, EChatType type, BOOL animate);

	/**
	 * Implements LLVoiceClientStatusObserver::onChange()
	 */
	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

protected:
	static BOOL matchChatTypeTrigger(const std::string& in_str, std::string* out_str);
	static void onChatBoxKeystroke(LLLineEditor* caller, void* userdata);
	static void onChatBoxFocusLost(LLFocusableElement* caller, void* userdata);

	void sendChat( EChatType type );
	void onChatBoxCommit();

	static LLWString stripChannelNumber(const LLWString &mesg, S32* channel);
	EChatType processChatTypeTriggers(EChatType type, std::string &str);

	// Which non-zero channel did we last chat on?
	static S32 sLastSpecialChatChannel;

	LLLineEditor*		mChatBox;
	LLTalkButton*		mTalkBtn;
};

#endif
