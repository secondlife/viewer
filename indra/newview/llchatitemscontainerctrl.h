/** 
 * @file llchatitemscontainer.h
 * @brief chat history scrolling panel implementation
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

#ifndef LL_LLCHATITEMSCONTAINER_H_
#define LL_LLCHATITEMSCONTAINER_H_

#include "llpanel.h"
#include "llscrollbar.h"
#include "string"
#include "llchat.h"

typedef enum e_show_item_header
{
	CHATITEMHEADER_SHOW_ONLY_NAME = 0,
	CHATITEMHEADER_SHOW_ONLY_ICON = 1,
	CHATITEMHEADER_SHOW_BOTH
} EShowItemHeader;

class LLChatItemCtrl: public LLPanel
{
protected:
	LLChatItemCtrl(){};
public:
	

	~LLChatItemCtrl(){}
	
	static LLChatItemCtrl* createInstance();

	void	draw();

	const LLChat& getMessage() const { return mOriginalMessage;}
	
	void	addText		(const std::string& message);
	void	setMessage	(const LLChat& msg);
	void	setWidth		(S32 width);
	void	snapToMessageHeight	();

	bool	canAddText	();

	void	onMouseLeave	(S32 x, S32 y, MASK mask);
	void	onMouseEnter	(S32 x, S32 y, MASK mask);
	BOOL	handleMouseDown	(S32 x, S32 y, MASK mask);

	virtual BOOL postBuild();

	void	reshape		(S32 width, S32 height, BOOL called_from_parent = TRUE);

	void	setHeaderVisibility(EShowItemHeader e);
	BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
private:
	
	std::string appendTime	();

private:
	LLChat mOriginalMessage;

	std::vector<std::string> mMessages;
};

class LLChatItemsContainerCtrl: public LLPanel
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLPanel::Params>
	{
		Params(){};
	};

	LLChatItemsContainerCtrl(const Params& params);


	~LLChatItemsContainerCtrl(){}

	void	addMessage	(const LLChat& msg);

	void	draw();

	void	reshape					(S32 width, S32 height, BOOL called_from_parent = TRUE);

	void	onScrollPosChangeCallback(S32, LLScrollbar*);

	virtual BOOL postBuild();

	BOOL	handleMouseDown	(S32 x, S32 y, MASK mask);
	BOOL	handleKeyHere	(KEY key, MASK mask);
	BOOL	handleScrollWheel( S32 x, S32 y, S32 clicks );

	void	scrollToBottom	();

	void	setHeaderVisibility(EShowItemHeader e);
	EShowItemHeader	getHeaderVisibility() const { return mEShowItemHeader;};


private:
	void	reformatHistoryScrollItems(S32 width);
	void	arrange					(S32 width, S32 height);

	S32		calcRecuiredHeight		();
	S32		getRecuiredHeight		() const { return mInnerRect.getHeight(); }

	void	updateLayout			(S32 width, S32 height);

	void	show_hide_scrollbar		(S32 width, S32 height);

	void	showScrollbar			(S32 width, S32 height);
	void	hideScrollbar			(S32 width, S32 height);

	void	panelSetLeftTopAndSize	(LLView* panel, S32 left, S32 top, S32 width, S32 height);
	void	panelShiftVertical		(LLView* panel,S32 delta);
	void	shiftPanels				(S32 delta);

private:
	std::vector<LLChatItemCtrl*> mItems;

	EShowItemHeader mEShowItemHeader;

	LLRect			mInnerRect;
	LLScrollbar*	mScrollbar;

};

#endif


