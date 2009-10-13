/** 
 * @file llimfloater.h
 * @brief LLIMFloater class definition
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_IMFLOATER_H
#define LL_IMFLOATER_H

#include "lldockablefloater.h"
#include "lllogchat.h"

class LLLineEditor;
class LLPanelChatControlPanel;
class LLViewerTextEditor;


/**
 * Individual IM window that appears at the bottom of the screen,
 * optionally "docked" to the bottom tray.
 */
class LLIMFloater : public LLDockableFloater
{
public:
	LLIMFloater(const LLUUID& session_id);

	virtual ~LLIMFloater();
	
	// LLView overrides
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void setVisible(BOOL visible);

	// LLFloater overrides
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void setDocked(bool docked, bool pop_on_undock = true);
	// override LLFloater's minimization according to EXT-1216

	// Make IM conversion visible and update the message history
	static LLIMFloater* show(const LLUUID& session_id);

	// Toggle panel specified by session_id
	// Returns true iff panel became visible
	static bool toggle(const LLUUID& session_id);

	// get new messages from LLIMModel
	void updateMessages();
	static void onSendMsg( LLUICtrl*, void*);
	void sendMsg();

	// callback for LLIMModel on new messages
	// route to specific floater if it is visible
	static void newIMCallback(const LLSD& data);

	// called when docked floater's position has been set by chiclet
	void setPositioned(bool b) { mPositioned = b; };

private:
	
	static void		onInputEditorFocusReceived( LLFocusableElement* caller, void* userdata );
	static void		onInputEditorFocusLost(LLFocusableElement* caller, void* userdata);
	static void		onInputEditorKeystroke(LLLineEditor* caller, void* userdata);
	void			setTyping(BOOL typing);
	void			onSlide();
	static void*	createPanelIMControl(void* userdata);
	static void*	createPanelGroupControl(void* userdata);
	// gets a rect that bounds possible positions for the LLIMFloater on a screen (EXT-1111)
	void getAllowedRect(LLRect& rect);

	static void chatFromLogFile(LLLogChat::ELogLineType type, std::string line, void* userdata);


	LLPanelChatControlPanel* mControlPanel;
	LLUUID mSessionID;
	S32 mLastMessageIndex;

	// username of last user who added text to this conversation, used to
	// suppress duplicate username divider bars
	std::string mLastFromName;

	EInstantMessage mDialog;
	LLUUID mOtherParticipantUUID;
	LLViewerTextEditor* mHistoryEditor;
	LLLineEditor* mInputEditor;
	bool mPositioned;
};


#endif  // LL_IMFLOATER_H
