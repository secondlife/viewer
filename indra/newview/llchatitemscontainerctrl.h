/** 
 * @file llchatitemscontainerctrl.h
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

#ifndef LL_LLCHATITEMSCONTAINERCTRL_H_
#define LL_LLCHATITEMSCONTAINERCTRL_H_

#include "llchat.h"
#include "llpanel.h"
#include "llscrollbar.h"
#include "llviewerchat.h"
#include "lltoastpanel.h"

typedef enum e_show_item_header
{
	CHATITEMHEADER_SHOW_ONLY_NAME = 0,
	CHATITEMHEADER_SHOW_ONLY_ICON = 1,
	CHATITEMHEADER_SHOW_BOTH
} EShowItemHeader;

class LLNearbyChatToastPanel: public LLToastPanelBase
{
protected:
        LLNearbyChatToastPanel()
		: 
	mIsDirty(false),
	mSourceType(CHAT_SOURCE_OBJECT)
	{};
public:
	~LLNearbyChatToastPanel(){}
	
	static LLNearbyChatToastPanel* createInstance();

	const LLUUID& getFromID() const { return mFromID;}
	
	//void	addText		(const std::string& message ,  const LLStyle::Params& input_params = LLStyle::Params());
	//void	setMessage	(const LLChat& msg);
	void	snapToMessageHeight	();

	bool	canAddText	();

	void	onMouseLeave	(S32 x, S32 y, MASK mask);
	void	onMouseEnter	(S32 x, S32 y, MASK mask);
	BOOL	handleMouseDown	(S32 x, S32 y, MASK mask);
	BOOL	handleMouseUp	(S32 x, S32 y, MASK mask);

	virtual BOOL postBuild();

	void	reshape		(S32 width, S32 height, BOOL called_from_parent = TRUE);

	void	setHeaderVisibility(EShowItemHeader e);
	BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);

	virtual void init(LLSD& data);
	virtual void addMessage(LLSD& data);

	virtual void draw();

	const LLUUID&	messageID() const { return mFromID;}
private:
	LLUUID			mFromID;	// agent id or object id
	EChatSourceType	mSourceType;
	


	bool mIsDirty;
};


#endif


