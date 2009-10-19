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

#include "llfloater.h"
#include "llscrollbar.h"
#include "llchat.h"

class LLResizeBar;
class LLChatHistory;

class LLNearbyChat: public LLFloater
{
public:
	// enumerations used by the chat system
	typedef enum e_chat_tearof_state
	{
		CHAT_PINNED = 0,
		CHAT_UNPINNED = 1,
	} EChatTearofState;

	enum { RESIZE_BAR_COUNT=4 };

	LLNearbyChat(const LLSD& key);
	~LLNearbyChat();

	BOOL	postBuild			();
	void	reshape				(S32 width, S32 height, BOOL called_from_parent = TRUE);
	
	BOOL	handleMouseDown		(S32 x, S32 y, MASK mask);
	BOOL	handleMouseUp		(S32 x, S32 y, MASK mask);
	BOOL	handleHover			(S32 x, S32 y, MASK mask);

	BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	
	void	addMessage			(const LLChat& message);
	
	void	onNearbySpeakers	();
	void	onTearOff();

	void	onNearbyChatContextMenuItemClicked(const LLSD& userdata);
	bool	onNearbyChatCheckContextMenuItem(const LLSD& userdata);

	/*virtual*/ void	onOpen	(const LLSD& key);

	/*virtual*/ void	draw	();

private:
	void	add_timestamped_line(const LLChat& chat, const LLColor4& color);
	
	void	pinn_panel();
	void	float_panel();

private:
	EChatTearofState mEChatTearofState;
	S32		mStart_X;
	S32		mStart_Y;

	LLHandle<LLView>	mPopupMenuHandle;
	LLPanel*			mChatCaptionPanel;
	LLChatHistory*		mChatHistory;

	bool				m_isDirty;
};

#endif


