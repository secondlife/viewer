/** 
 * @file llnearbychat.h
 * @brief nearby chat history scrolling panel implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLNEARBYCHAT_H_
#define LL_LLNEARBYCHAT_H_

#include "lldockablefloater.h"
#include "llscrollbar.h"
#include "llviewerchat.h"

class LLResizeBar;
class LLChatHistory;

class LLNearbyChat: public LLDockableFloater
{
public:
	LLNearbyChat(const LLSD& key);
	~LLNearbyChat();

	BOOL	postBuild			();

	/** @param archive true - to save a message to the chat history log */
	void	addMessage			(const LLChat& message,bool archive = true, const LLSD &args = LLSD());	
	void	onNearbyChatContextMenuItemClicked(const LLSD& userdata);
	bool	onNearbyChatCheckContextMenuItem(const LLSD& userdata);

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);

	// focus overrides
	/*virtual*/ void	onFocusLost();
	/*virtual*/ void	onFocusReceived();
	
	/*virtual*/ void	onOpen	(const LLSD& key);

	/*virtual*/ void	setVisible(BOOL visible);

	virtual void setRect		(const LLRect &rect);

	virtual void updateChatHistoryStyle();

	static void processChatHistoryStyleUpdate(const LLSD& newvalue);

	void loadHistory();

	static LLNearbyChat* getInstance();

private:
	virtual void    applySavedVariables();

	void	getAllowedRect		(LLRect& rect);

	void	onNearbySpeakers	();
	

private:
	LLHandle<LLView>	mPopupMenuHandle;
	LLChatHistory*		mChatHistory;

	std::vector<LLChat> mMessageArchive;
};

#endif


